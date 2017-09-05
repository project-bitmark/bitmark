#include "chainparams.h"
#include "coins.h"
#include "main.h"
#include "uint256.h"
#include "script.h"
#include "core.h"
#include "bignum.h"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_SUITE(auxpow_tests)

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
    inline CAuxPow
    get() const
    {
        assert(!parentBlock.vtx.empty());
        return get(parentBlock.vtx[0]);
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

void
CAuxpowBuilder::setCoinbase(const CScript& scr)
{
    CTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout.SetNull();
    mtx.vin[0].scriptSig = scr;

    parentBlock.vtx.clear();
    parentBlock.vtx.push_back(mtx);
    parentBlock.hashMerkleRoot = parentBlock.BuildMerkleTree();
}

std::vector<unsigned char>
CAuxpowBuilder::buildAuxpowChain(const uint256& hashAux, unsigned h, int index)
{
    auxpowChainIndex = index;

    /* Just use "something" for the branch.  Doesn't really matter.  */
    auxpowChainMerkleBranch.clear();
    for (unsigned i = 0; i < h; ++i)
      auxpowChainMerkleBranch.push_back(CBigNum(i).getuint256());

    const uint256 hash = CBlock::CheckMerkleBranch(hashAux, auxpowChainMerkleBranch, index);

    std::vector<unsigned char> res = ToByteVector(hash);
    std::reverse(res.begin(), res.end());

    return res;
}

CAuxPow
CAuxpowBuilder::get(const CTransaction& tx) const
{
    LOCK(cs_main);
    CAuxPow res(tx);
    res.SetMerkleBranch(&parentBlock);

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

static void
mineBlock(CBlockHeader& block, bool ok, int nBits = -1)
{
  if (nBits == -1)
    nBits = block.nBits;

  CBigNum target;
  target.SetCompact(nBits);
  int algo = block.GetAlgo();
  block.nNonce = 0;
  while (true) {
    const bool nowOk = (CBigNum(block.GetPoWHash(algo)) <= target);
    if ((ok && nowOk) || (!ok && !nowOk))
      break;

    ++block.nNonce;
  }

  if (ok)
    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(algo), nBits));
  else
    BOOST_CHECK(!CheckProofOfWork(block.GetPoWHash(algo), nBits));
}
  
BOOST_AUTO_TEST_CASE(auxpow_pow)
{
  SelectParams(CChainParams::REGTEST);
  CBigNum target = CBigNum(~uint256(0) >> 1);
  CBlockHeader block;
  block.nBits = target.GetCompact();

  CAuxpowBuilder builder(2,42);
  CAuxPow auxpow;
  const int16_t ourChainId = Params().GetAuxpowChainId();
  const unsigned height = 3;
  const int nonce = 7;
  const int index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
  std::vector<unsigned char> auxRoot, data;

  block.SetAuxpow(true);
  block.SetChainId(ourChainId);
  auxRoot = builder.buildAuxpowChain(block.GetHash(), height, index);
  data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
  builder.setCoinbase(CScript() << data);
  mineBlock(builder.parentBlock, false, block.nBits);
  block.SetAuxpow(new CAuxPow(builder.get()));
  BOOST_CHECK(!CheckAuxPowProofOfWork(block,Params()));
  mineBlock(builder.parentBlock, true, block.nBits);
  block.SetAuxpow(new CAuxPow(builder.get()));
  BOOST_CHECK(CheckAuxPowProofOfWork(block,Params()));
  
}

BOOST_AUTO_TEST_SUITE_END()
