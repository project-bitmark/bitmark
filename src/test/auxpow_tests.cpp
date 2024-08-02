#include "arith_uint256.h"
#include "chainparams.h"
#include "coins.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "uint256.h"
#include <kernel/cs_main.h>
#include <pow.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_SUITE(auxpow_tests)

static void
tamperWith(uint256& num)
{
    arith_uint256 modifiable = UintToArith256(num);
    modifiable += 1;
    num = ArithToUint256(modifiable);
}

/**
 * Utility class to construct auxpow's and manipulate them.  This is used
 * to simulate various scenarios.
 */
class CAuxpowBuilder
{
public:
    /** The parent block (with coinbase, not just header).  */
    CBlock parentBlock;

    /** The auxpow's merkle branch (connecting it to the coinbase).  */
    std::vector<uint256> auxpowChainMerkleBranch;
    /** The auxpow's merkle tree index.  */
    int auxpowChainIndex;

    /**
     * Initialise everything.
     * @param baseVersion The parent block's base version to use.
     * @param chainId The parent block's chain ID to use.
     */
    CAuxpowBuilder(int baseVersion, int16_t chainId);

    /**
     * Set the coinbase's script.
     * @param scr Set it to this script.
     */
    void setCoinbase(const CScript& scr);

    /**
     * Build the auxpow merkle branch.  The member variables will be
     * set accordingly.  This has to be done before constructing the coinbase
     * itself (which must contain the root merkle hash).  When we have the
     * coinbase afterwards, the member variables can be used to initialise
     * the CAuxPow object from it.
     * @param hashAux The merge-mined chain's block hash.
     * @param h Height of the merkle tree to build.
     * @param index Index to use in the merkle tree.
     * @return The root hash, with reversed endian.
     */
    std::vector<unsigned char> buildAuxpowChain(const uint256& hashAux, unsigned h, int index);

    /**
     * Build the finished CAuxPow object.  We assume that the auxpowChain
     * member variables are already set.  We use the passed in transaction
     * as the base.  It should (probably) be the parent block's coinbase.
     * @param tx The base tx to use.
     * @return The constructed CAuxPow object.
     */
    CAuxPow get(const CTransaction& tx) const;

    /**
     * Build the finished CAuxPow object from the parent block's coinbase.
     * @return The constructed CAuxPow object.
     */
    inline CAuxPow get() const
    {
        assert(!parentBlock.vtx.empty());
        return get(*(parentBlock.vtx[0]));
    }

    /**
     * Build a data vector to be included in the coinbase.  It consists
     * of the aux hash, the merkle tree size and the nonce.  Optionally,
     * the header can be added as well.
     * @param header Add the header?
     * @param hashAux The aux merkle root hash.
     * @param h Height of the merkle tree.
     * @param nonce The nonce value to use.
     * @return The constructed data.
     */
    static std::vector<unsigned char> buildCoinbaseData(bool header, const std::vector<unsigned char>& auxRoot, unsigned h, int nonce);
};

CAuxpowBuilder::CAuxpowBuilder(int baseVersion, int16_t chainId)
    : auxpowChainIndex(-1)
{
    parentBlock.nVersion = baseVersion;
    parentBlock.SetChainId(chainId);
}

void CAuxpowBuilder::setCoinbase(const CScript& scr)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout.SetNull();
    mtx.vin[0].scriptSig = scr;

    parentBlock.vtx.clear();
    parentBlock.vtx.push_back(MakeTransactionRef(mtx));
    parentBlock.hashMerkleRoot = parentBlock.BuildMerkleTree();
}

std::vector<unsigned char>
CAuxpowBuilder::buildAuxpowChain(const uint256& hashAux, unsigned h, int index)
{
    auxpowChainIndex = index;

    /* Just use "something" for the branch.  Doesn't really matter.  */
    auxpowChainMerkleBranch.clear();
    for (unsigned i = 0; i < h; ++i)
        auxpowChainMerkleBranch.push_back(uint256::ONE);

    const uint256 hash = CBlock::CheckMerkleBranch(hashAux, auxpowChainMerkleBranch, index);

    std::vector<unsigned char> res = ToByteVector(hash);
    std::reverse(res.begin(), res.end());

    return res;
}

CAuxPow CAuxpowBuilder::get(const CTransaction& tx) const
{
    LOCK(cs_main);
    CAuxPow res = CAuxPow(CMutableTransaction(tx));

    //auto hashBlock = parentBlock.GetHash();

    ////std::cout << parentBlock.vtx[0] << std::endl
    ////          << (CTransaction&)res << std::endl;

    //// Locate the transaction
    //for (res.nIndex = 0; res.nIndex < (int)parentBlock.vtx.size(); res.nIndex++)
    //    if (*(parentBlock.vtx[res.nIndex]) == CTransaction(res))
    //        break;

    //// Fill in merkle branch
    //res.vMerkleBranch = parentBlock.GetMerkleBranch(res.nIndex);

    res.SetMerkleBranch(parentBlock);

    res.vChainMerkleBranch = auxpowChainMerkleBranch;
    res.nChainIndex = auxpowChainIndex;
    res.parentBlock = parentBlock;

    return res;
}

std::vector<unsigned char>
CAuxpowBuilder::buildCoinbaseData(bool header, const std::vector<unsigned char>& auxRoot, unsigned h, int nonce)
{
    std::vector<unsigned char> res;

    if (header)
        res.insert(res.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));
    res.insert(res.end(), auxRoot.begin(), auxRoot.end());

    const int size = (1 << h);
    res.insert(res.end(), UBEGIN(size), UEND(size));
    res.insert(res.end(), UBEGIN(nonce), UEND(nonce));

    return res;
}

static void mineBlock(CBlockHeader& block, bool ok, int nBits = -1)
{
    if (nBits == -1)
        nBits = block.nBits;

    arith_uint256 target;
    target.SetCompact(nBits);
    Algo algo = block.GetAlgo();
    block.nNonce = 0;
    while (true) {
        const bool nowOk = (UintToArith256(block.GetPoWHash(algo)) <= target);
        if ((ok && nowOk) || (!ok && !nowOk))
            break;

        ++block.nNonce;
    }

    if (ok)
        BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(algo), nBits, block.GetAlgo()));
    else
        BOOST_CHECK(!CheckProofOfWork(block.GetPoWHash(algo), nBits, block.GetAlgo()));
}

// Constant stuff for coinbase transactions we create:
// CScript COINBASE_FLAGS;

BOOST_AUTO_TEST_CASE(check_auxpow)
{
    SelectParams(ChainType::REGTEST);

    CAuxpowBuilder builder(2, 42);
    CAuxPow auxpow;

    const uint256 hashAux = ArithToUint256(arith_uint256(12345));
    const int32_t ourChainId = Params().GetConsensus().nAuxpowChainId;
    const unsigned height = 30;
    const int nonce = 7;
    int index;

    std::vector<unsigned char> auxRoot, data;
    CScript scr;

    /* Build a correct auxpow.  The height is the maximally allowed one.  */
    index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    auxRoot = builder.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    scr = (CScript() << 2809 << 2013); // TODO: check +COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    builder.setCoinbase(scr);
    BOOST_CHECK(builder.get().check(hashAux, ourChainId));

    /* Check that the auxpow is invalid if we change either the aux block's
       hash or the chain ID.  */
    uint256 modifiedAux(hashAux);
    tamperWith(modifiedAux);
    BOOST_CHECK(!builder.get().check(modifiedAux, ourChainId));
    BOOST_CHECK(!builder.get().check(hashAux, ourChainId + 1));

    /* Non-coinbase parent tx should fail.  Note that we can't just copy
       the coinbase literally, as we have to get a tx with different hash.  */
    const CTransactionRef oldCoinbase = builder.parentBlock.vtx[0];
    builder.setCoinbase(scr << 5);
    builder.parentBlock.vtx.push_back(oldCoinbase);
    builder.parentBlock.hashMerkleRoot = builder.parentBlock.BuildMerkleTree();
    auxpow = builder.get(*builder.parentBlock.vtx[0]);
    BOOST_CHECK(auxpow.check(hashAux, ourChainId));
    auxpow = builder.get(*builder.parentBlock.vtx[1]);
    BOOST_CHECK(!auxpow.check(hashAux, ourChainId));

    /* The parent chain can't have the same chain ID.  */
    CAuxpowBuilder builder2(builder);
    builder2.parentBlock.SetChainId(150);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));
    builder2.parentBlock.SetChainId(ourChainId);
    // BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    /* Disallow too long merkle branches.  */
    builder2 = builder;
    index = CAuxPow::getExpectedIndex(nonce, ourChainId, height + 1);
    auxRoot = builder2.buildAuxpowChain(hashAux, height + 1, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height + 1, nonce);
    scr = (CScript() << 2809 << 2013); // + COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    builder2.setCoinbase(scr);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    /* Verify that we compare correctly to the parent block's merkle root.  */
    builder2 = builder;
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));
    tamperWith(builder2.parentBlock.hashMerkleRoot);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    /* Build a non-header legacy version and check that it is also accepted.  */
    builder2 = builder;
    index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    auxRoot = builder2.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(false, auxRoot, height, nonce);
    scr = (CScript() << 2809 << 2013); // + COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    builder2.setCoinbase(scr);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));

    /* However, various attempts at smuggling two roots in should be detected.  */

    const std::vector<unsigned char> wrongAuxRoot = builder2.buildAuxpowChain(modifiedAux, height, index);
    std::vector<unsigned char> data2 = CAuxpowBuilder::buildCoinbaseData(false, wrongAuxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data << data2);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));
    builder2.setCoinbase(CScript() << data2 << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    data2 = CAuxpowBuilder::buildCoinbaseData(true, wrongAuxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data << data2);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));
    builder2.setCoinbase(CScript() << data2 << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data << data2);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));
    builder2.setCoinbase(CScript() << data2 << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    data2 = CAuxpowBuilder::buildCoinbaseData(false, wrongAuxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data << data2);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));
    builder2.setCoinbase(CScript() << data2 << data);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));

    /* Verify that the appended nonce/size values are checked correctly.  */

    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));

    data.pop_back();
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height - 1, nonce);
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce + 3);
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    /* Put the aux hash in an invalid merkle tree position.  */

    auxRoot = builder.buildAuxpowChain(hashAux, height, index + 1);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder2.get().check(hashAux, ourChainId));

    auxRoot = builder.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder2.setCoinbase(CScript() << data);
    BOOST_CHECK(builder2.get().check(hashAux, ourChainId));
}

BOOST_AUTO_TEST_CASE(auxpow_pow)
{
    arith_uint256 target = arith_uint256(~arith_uint256(0) >> 1);
    CBlockHeader block;
    block.nBits = target.GetCompact();

    /* Check the case when the block does not have auxpow (this is true right now).  */

    block.SetChainId(Params().GetConsensus().nAuxpowChainId);
    block.SetAuxpow(true);
    mineBlock(block, true);
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    block.SetAuxpow(false);
    mineBlock(block, true);
    BOOST_CHECK(CheckAuxPowProofOfWork(block));
    mineBlock(block, false);
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    /* Check the case that the block has auxpow.  */

    CAuxpowBuilder builder(2, 42);
    CAuxPow auxpow;
    const int16_t ourChainId = Params().GetConsensus().nAuxpowChainId;
    const unsigned height = 3;
    const int nonce = 7;
    const int index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    std::vector<unsigned char> auxRoot, data;

    /* Valid auxpow, PoW check of parent block.  */
    block.SetAuxpow(true);
    block.SetChainId(ourChainId);
    auxRoot = builder.buildAuxpowChain(block.GetHash(), height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    mineBlock(builder.parentBlock, false, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));
    mineBlock(builder.parentBlock, true, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(CheckAuxPowProofOfWork(block));

    /*  Mismatch between auxpow being present and block.nVersion. */
    block.SetAuxpow(false);
    const uint256 hashAux = block.GetHash();
    auxRoot = builder.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    mineBlock(builder.parentBlock, true, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(hashAux != block.GetHash());
    block.SetAuxpow(false);
    BOOST_CHECK(hashAux == block.GetHash());
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    /* Modifying the block invalidates the PoW.  */
    block.SetAuxpow(true);
    auxRoot = builder.buildAuxpowChain(block.GetHash(), height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    mineBlock(builder.parentBlock, true, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(CheckAuxPowProofOfWork(block));
    tamperWith(block.hashMerkleRoot);
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_SUITE_END()
