// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <primitives/block.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <node/miner.h>
#include <pow.h>
#include <random.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>
#include <util/strencodings.h>

#include <thread>

using node::BlockAssembler;

namespace validation_block_tests {
struct MinerTestingSetup : public RegTestingSetup {
    std::shared_ptr<CBlock> Block(const uint256& prev_hash);
    std::shared_ptr<const CBlock> GoodBlock(const uint256& prev_hash);
    std::shared_ptr<const CBlock> BadBlock(const uint256& prev_hash);
    std::shared_ptr<CBlock> FinalizeBlock(std::shared_ptr<CBlock> pblock);
    void BuildChain(const uint256& root, int height, const unsigned int invalid_rate, const unsigned int branch_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks);
};
} // namespace validation_block_tests

BOOST_FIXTURE_TEST_SUITE(validation_block_tests, MinerTestingSetup)

struct TestSubscriber final : public CValidationInterface {
    uint256 m_expected_tip;

    explicit TestSubscriber(uint256 tip) : m_expected_tip(tip) {}

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, pindexNew->GetBlockHash());
    }

    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->hashPrevBlock);
        BOOST_CHECK_EQUAL(m_expected_tip, pindex->pprev->GetBlockHash());

        m_expected_tip = block->GetHash();
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->GetHash());
        BOOST_CHECK_EQUAL(m_expected_tip, pindex->GetBlockHash());

        m_expected_tip = block->hashPrevBlock;
    }
};

std::shared_ptr<CBlock> MinerTestingSetup::Block(const uint256& prev_hash)
{
    static int i = 0;
    static uint64_t time = Params().GenesisBlock().nTime;

    auto ptemplate = BlockAssembler{m_node.chainman->ActiveChainstate(), m_node.mempool.get()}.CreateNewBlock(CScript{} << i++ << OP_TRUE);
    auto pblock = std::make_shared<CBlock>(ptemplate->block);
    pblock->hashPrevBlock = prev_hash;
    pblock->nTime = ++time;

    // Make the coinbase transaction with two outputs:
    // One zero-value one that has a unique pubkey to make sure that blocks at the same height can have a different hash
    // Another one that has the coinbase reward in a P2WSH with OP_TRUE as witness program to make it easy to spend
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vout.resize(2);
    txCoinbase.vout[1].scriptPubKey = P2WSH_OP_TRUE;
    txCoinbase.vout[1].nValue = txCoinbase.vout[0].nValue;
    txCoinbase.vout[0].nValue = 0;
    txCoinbase.vin[0].scriptWitness.SetNull();
    // Always pad with OP_0 at the end to avoid bad-cb-length error
    txCoinbase.vin[0].scriptSig = CScript{} << WITH_LOCK(::cs_main, return m_node.chainman->m_blockman.LookupBlockIndex(prev_hash)->nHeight + 1) << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    return pblock;
}

std::shared_ptr<CBlock> MinerTestingSetup::FinalizeBlock(std::shared_ptr<CBlock> pblock)
{
    const CBlockIndex* prev_block{WITH_LOCK(::cs_main, return m_node.chainman->m_blockman.LookupBlockIndex(pblock->hashPrevBlock))};
    m_node.chainman->GenerateCoinbaseCommitment(*pblock, prev_block);

    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    while (!CheckProofOfWork(pblock->GetPoWHash(), pblock->nBits, pblock->GetAlgo())) {
        ++(pblock->nNonce);
    }

    // submit block header, so that miner can get the block height from the
    // global state and the node has the topology of the chain
    BlockValidationState ignored;
    BOOST_CHECK(Assert(m_node.chainman)->ProcessNewBlockHeaders({pblock->GetBlockHeader()}, true, ignored));

    return pblock;
}

// construct a valid block
std::shared_ptr<const CBlock> MinerTestingSetup::GoodBlock(const uint256& prev_hash)
{
    return FinalizeBlock(Block(prev_hash));
}

// construct an invalid block (but with a valid header)
std::shared_ptr<const CBlock> MinerTestingSetup::BadBlock(const uint256& prev_hash)
{
    auto pblock = Block(prev_hash);

    CMutableTransaction coinbase_spend;
    coinbase_spend.vin.emplace_back(COutPoint(pblock->vtx[0]->GetHash(), 0), CScript(), 0);
    coinbase_spend.vout.push_back(pblock->vtx[0]->vout[0]);

    CTransactionRef tx = MakeTransactionRef(coinbase_spend);
    pblock->vtx.push_back(tx);

    auto ret = FinalizeBlock(pblock);
    return ret;
}

void MinerTestingSetup::BuildChain(const uint256& root, int height, const unsigned int invalid_rate, const unsigned int branch_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    if (height <= 0 || blocks.size() >= max_size) return;

    bool gen_invalid = InsecureRandRange(100) < invalid_rate;
    bool gen_fork = InsecureRandRange(100) < branch_rate;

    const std::shared_ptr<const CBlock> pblock = gen_invalid ? BadBlock(root) : GoodBlock(root);
    blocks.push_back(pblock);
    if (!gen_invalid) {
        BuildChain(pblock->GetHash(), height - 1, invalid_rate, branch_rate, max_size, blocks);
    }

    if (gen_fork) {
        blocks.push_back(GoodBlock(root));
        BuildChain(blocks.back()->GetHash(), height - 1, invalid_rate, branch_rate, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblock_signals_ordering)
{
    // build a large-ish chain that's likely to have some forks
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < 50) {
        blocks.clear();
        BuildChain(Params().GenesisBlock().GetHash(), 100, 15, 10, 500, blocks);
    }

    bool ignored;
    // Connect the genesis block and drain any outstanding events
    BOOST_CHECK(Assert(m_node.chainman)->ProcessNewBlock(std::make_shared<CBlock>(Params().GenesisBlock()), true, true, &ignored));
    SyncWithValidationInterfaceQueue();

    // subscribe to events (this subscriber will validate event ordering)
    const CBlockIndex* initial_tip = nullptr;
    {
        LOCK(cs_main);
        initial_tip = m_node.chainman->ActiveChain().Tip();
    }
    auto sub = std::make_shared<TestSubscriber>(initial_tip->GetBlockHash());
    RegisterSharedValidationInterface(sub);

    // create a bunch of threads that repeatedly process a block generated above at random
    // this will create parallelism and randomness inside validation - the ValidationInterface
    // will subscribe to events generated during block validation and assert on ordering invariance
    std::vector<std::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&]() {
            bool ignored;
            FastRandomContext insecure;
            for (int i = 0; i < 1000; i++) {
                auto block = blocks[insecure.randrange(blocks.size() - 1)];
                Assert(m_node.chainman)->ProcessNewBlock(block, true, true, &ignored);
            }

            // to make sure that eventually we process the full chain - do it here
            for (const auto& block : blocks) {
                if (block->vtx.size() == 1) {
                    bool processed = Assert(m_node.chainman)->ProcessNewBlock(block, true, true, &ignored);
                    //assert(processed);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    SyncWithValidationInterfaceQueue();

    UnregisterSharedValidationInterface(sub);

    LOCK(cs_main);
    BOOST_CHECK_EQUAL(sub->m_expected_tip, m_node.chainman->ActiveChain().Tip()->GetBlockHash());
}

/**
 * Test that mempool updates happen atomically with reorgs.
 *
 * This prevents RPC clients, among others, from retrieving immediately-out-of-date mempool data
 * during large reorgs.
 *
 * The test verifies this by creating a chain of `num_txs` blocks, matures their coinbases, and then
 * submits txns spending from their coinbase to the mempool. A fork chain is then processed,
 * invalidating the txns and evicting them from the mempool.
 *
 * We verify that the mempool updates atomically by polling it continuously
 * from another thread during the reorg and checking that its size only changes
 * once. The size changing exactly once indicates that the polling thread's
 * view of the mempool is either consistent with the chain state before reorg,
 * or consistent with the chain state after the reorg, and not just consistent
 * with some intermediate state during the reorg.
 */
BOOST_AUTO_TEST_CASE(mempool_locks_reorg)
{
    bool ignored;
    auto ProcessBlock = [&](std::shared_ptr<const CBlock> block) -> bool {
        return Assert(m_node.chainman)->ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&ignored);
    };

    // Process all mined blocks
    BOOST_REQUIRE(ProcessBlock(std::make_shared<CBlock>(Params().GenesisBlock())));
    auto last_mined = GoodBlock(Params().GenesisBlock().GetHash());
    BOOST_REQUIRE(ProcessBlock(last_mined));

    // Run the test multiple times
    for (int test_runs = 3; test_runs > 0; --test_runs) {
        BOOST_CHECK_EQUAL(last_mined->GetHash(), WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip()->GetBlockHash()));

        // Later on split from here
        const uint256 split_hash{last_mined->hashPrevBlock};

        // Create a bunch of transactions to spend the miner rewards of the
        // most recent blocks
        std::vector<CTransactionRef> txs;
        for (int num_txs = 22; num_txs > 0; --num_txs) {
            CMutableTransaction mtx;
            mtx.vin.emplace_back(COutPoint{last_mined->vtx[0]->GetHash(), 1}, CScript{});
            mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
            mtx.vout.push_back(last_mined->vtx[0]->vout[1]);
            mtx.vout[0].nValue -= 1000;
            txs.push_back(MakeTransactionRef(mtx));

            last_mined = GoodBlock(last_mined->GetHash());
            BOOST_REQUIRE(ProcessBlock(last_mined));
        }

        // Mature the inputs of the txs
        for (int j = COINBASE_MATURITY; j > 0; --j) {
            last_mined = GoodBlock(last_mined->GetHash());
            BOOST_REQUIRE(ProcessBlock(last_mined));
        }

        // Mine a reorg (and hold it back) before adding the txs to the mempool
        const uint256 tip_init{last_mined->GetHash()};

        std::vector<std::shared_ptr<const CBlock>> reorg;
        last_mined = GoodBlock(split_hash);
        reorg.push_back(last_mined);
        for (size_t j = COINBASE_MATURITY + txs.size() + 1; j > 0; --j) {
            last_mined = GoodBlock(last_mined->GetHash());
            reorg.push_back(last_mined);
        }

        // Add the txs to the tx pool
        {
            LOCK(cs_main);
            for (const auto& tx : txs) {
                const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(tx);
                BOOST_REQUIRE(result.m_result_type == MempoolAcceptResult::ResultType::VALID);
            }
        }

        // Check that all txs are in the pool
        {
            BOOST_CHECK_EQUAL(m_node.mempool->size(), txs.size());
        }

        // Run a thread that simulates an RPC caller that is polling while
        // validation is doing a reorg
        std::thread rpc_thread{[&]() {
            // This thread is checking that the mempool either contains all of
            // the transactions invalidated by the reorg, or none of them, and
            // not some intermediate amount.
            while (true) {
                LOCK(m_node.mempool->cs);
                if (m_node.mempool->size() == 0) {
                    // We are done with the reorg
                    break;
                }
                // Internally, we might be in the middle of the reorg, but
                // externally the reorg to the most-proof-of-work chain should
                // be atomic. So the caller assumes that the returned mempool
                // is consistent. That is, it has all txs that were there
                // before the reorg.
                assert(m_node.mempool->size() == txs.size());
                continue;
            }
            LOCK(cs_main);
            // We are done with the reorg, so the tip must have changed
            assert(tip_init != m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }};

        // Submit the reorg in this thread to invalidate and remove the txs from the tx pool
        for (const auto& b : reorg) {
            ProcessBlock(b);
        }
        // Check that the reorg was eventually successful
        BOOST_CHECK_EQUAL(last_mined->GetHash(), WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip()->GetBlockHash()));

        // We can join the other thread, which returns when the reorg was successful
        rpc_thread.join();
    }
}

BOOST_AUTO_TEST_CASE(witness_commitment_index)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    CScript pubKey;
    pubKey << 1 << OP_TRUE;
    auto ptemplate = BlockAssembler{m_node.chainman->ActiveChainstate(), m_node.mempool.get()}.CreateNewBlock(pubKey);
    CBlock pblock = ptemplate->block;

    CTxOut witness;
    witness.scriptPubKey.resize(MINIMUM_WITNESS_COMMITMENT);
    witness.scriptPubKey[0] = OP_RETURN;
    witness.scriptPubKey[1] = 0x24;
    witness.scriptPubKey[2] = 0xaa;
    witness.scriptPubKey[3] = 0x21;
    witness.scriptPubKey[4] = 0xa9;
    witness.scriptPubKey[5] = 0xed;

    // A witness larger than the minimum size is still valid
    CTxOut min_plus_one = witness;
    min_plus_one.scriptPubKey.resize(MINIMUM_WITNESS_COMMITMENT + 1);

    CTxOut invalid = witness;
    invalid.scriptPubKey[0] = OP_VERIFY;

    CMutableTransaction txCoinbase(*pblock.vtx[0]);
    txCoinbase.vout.resize(4);
    txCoinbase.vout[0] = witness;
    txCoinbase.vout[1] = witness;
    txCoinbase.vout[2] = min_plus_one;
    txCoinbase.vout[3] = invalid;
    pblock.vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    BOOST_CHECK_EQUAL(GetWitnessCommitmentIndex(pblock), 2);
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_scrypt)
{
    CBlock block;

    // main chain #451999 block
    DataStream ds{ParseHex("04000000c8262e3b05e40a3835db4712d832c54c465ab35388571b2da363c624270b000050a7cae479cee3c9f3601f2d2b14b5bfd3f4df19ae33232cac9ab8f02100d3985fc91a5b121c0e1b09d36db00102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0c039be5063494fc01c70000000000000001002f6859000000001976a914f330c69d5af89893d6e54108d3c9c6e51478d7fa88ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::SCRYPT);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("fcef26625871f83ceb06abfdd5cc227b8dfda100fd98cee46ad263338448762b"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("00000000000c17e1fb526e04cbfdbd6a7674af643c5e8a5d260ec0d765d96974"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("00000b2724c663a32d1b578853b35a464cc532d81247db35380ae4053b2e26c8"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("98d30021f0b89aac2c2333ae19dff4d3bfb5142b2d1f60f3c9e3ce79e4caa750"));
    BOOST_CHECK_EQUAL(block.nVersion, 4);
    BOOST_CHECK_EQUAL(block.nTime, 1528482143);
    BOOST_CHECK_EQUAL(block.nBits, 0x1b0e1c12);
    BOOST_CHECK_EQUAL(block.nNonce, 2959987465);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_scrypt_aux)
{
    CBlock block;

    // main chain #461996 block
    DataStream ds{ParseHex("04015b00918edcb2034b69fe33bbeeb18d6315dd9e1e22b3c08e461997df5c7c8cb3f798f239dc49e74336b94a09cc331459c5b4acc301527cfe21ff42de858779fede4bd5e72c5b9d8b001b0000000001000000010000000000000000000000000000000000000000000000000000000000000000ffffffff47030b2e68043ae82c5b0881012cab010000007a706f6f6c2e636100fabe6d6de37d769d5a38323a2422af3ece47c1b06860d37f5456b1ee3f28345c9797acbc100000000000000000000000013b84c1ee110000001976a9141e075b9f07c4230c00c08186fb699513c00662e388ac0000000000000000000075a36583f48e7ec9bcdddd311b58ffc08eca4025ee756a59062a01311ec2dcde627261db43acd1c6cd83ba63b550f1c3e016da6c8a7c33e811b1c50000000004ec9f00532b366ff859d60caf8d2ac63464fbaf9f9ec4b354d8885b19308a46a5b887da16db0da604bc3b5acf9bbe039b2173cd0e7d8669e7bc5d0ee6302be0bf313ae37feca56d88660a428cf96c8f892654bfed477ab949b8ace54a5846151b21f5582f55fcd71618611c7d62b1444ccc3ab1033e083b13296a6dac507e17d40d00000002000020ab1e97dea1bdd599a5aaba2784d7228e4b28508e6757b4780cd92e4e6f40075af90b553da10f5bde88caecbd5ad9124bd8cc60f7ea9d6dee7cd0eb988a36c5f53ae82c5b630b011b8270f2170101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603ac0c070101ffffffff01002f6859000000002321035c8d5504abb9d3d4609ca649e702540cbf0c3623b7d828fdb0693b8db95f808dac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::SCRYPT);
    BOOST_CHECK_EQUAL(block.IsAuxpow(), true);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("603e5a2baeacf129da77d8c83cc2106600068c214ca43b1c5146cf8861ae7fe5"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("914adcd1998cbd76e7641c2bc8657c4c7f37012ca0860b1ade547f10f0abd985"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("98f7b38c7c5cdf9719468ec0b3221e9edd15638db1eebb33fe694b03b2dc8e91"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("4bdefe798785de42ff21fe7c5201c3acb4c5591433cc094ab93643e749dc39f2"));
    BOOST_CHECK_EQUAL(block.nVersion, 5964036);
    BOOST_CHECK_EQUAL(block.nTime, 1529669589);
    BOOST_CHECK_EQUAL(block.nBits, 0x1b008b9d);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_sha256D)
{
    CBlock block;

    // main chain #451999 argon block
    DataStream ds{ParseHex("040200000f5e615f3bf23ed7b5b48d0a92449425a6115af7dfd2e3bf7575222314b67bfbaccc6ea89738037d6c40dfbaef32af015f08b59afb453e4c31ef4480ff9b621a2bcf1a5b4bdc7119a405bcf00102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0c039fe506661ca6025b8307000000000001002f6859000000001976a914f330c69d5af89893d6e54108d3c9c6e51478d7fa88ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::SHA256D);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("00000000000000279af4acbe06d264a54395a6a29ade8a2dcb505026b9e4cc68"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("00000000000000279af4acbe06d264a54395a6a29ade8a2dcb505026b9e4cc68"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("fb7bb61423227575bfe3d2dff75a11a6259444920a8db4b5d73ef23b5f615e0f"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("1a629bff8044ef314c3e45fb9ab5085f01af32efbadf406c7d033897a86eccac"));
    BOOST_CHECK_EQUAL(block.nVersion, 516);
    BOOST_CHECK_EQUAL(block.nTime, 1528483627);
    BOOST_CHECK_EQUAL(block.nBits, 0x1971dc4b);
    BOOST_CHECK_EQUAL(block.nNonce, 4038854052);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_sha256D_aux)
{
    CBlock block;

    // main chain #462001 block
    DataStream ds{ParseHex("04035b0008f4129310262ab3b55a8f925bf6fdea2757fc1a71196b6dbc8e775ee022645e48811cd00170254ec87bda4433e31f7a47ca17a5e5e74ef4403500b994cd1805baea2c5b62b314190000000002000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4403372e6804b6eb2c5b08810001ef889a00007969696d7000fabe6d6da4bb6be898443f264aea45840975cb55f2cc3ac9adfc2e253b50d3353c498262020000000000000000000000010fbabcee110000001976a9141bc6b3fcc77e7e84ce0fe2e2b98c41b2fcd6ca6f88ac00000000000000000000001219d5aa13ca4047e874fe699f5f2134b8d8318fecbaa9903e02fb7bbd41e1c4d3ace66fea4f6ce3a5f37fdbaf67215926cbd89115e74818bf0003baad74c572bad00a0e3a56454025d13244f9b66aec9757ec6b494aa961453b00000000013845407e59f168e24f6939f4c897c1bdded233a24e7f5fc3ae0ed2f307725c0d010000000202002022e9309871736c78614cd607ef93592155f44457265fd49d40f7b5ff9a7293ffcddfe10b1d45e355550014670110236d980477690b3d2e0feb804b7ef7bf5deab6eb2c5bf155031942002ab30101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603b10c070101ffffffff01573c024d00000000232102c5994ae873a89b7a74a59a76008acf6fccdb7d1208ffaa4524a4220886ff20eeac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::SHA256D);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("5a579d0d1ab8c6a52516784c02d87aba5d89cf4f22526d3abd22a1d6541a2a2c"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("5a579d0d1ab8c6a52516784c02d87aba5d89cf4f22526d3abd22a1d6541a2a2c"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("5e6422e05e778ebc6d6b19711afc5727eafdf65b928f5ab5b32a26109312f408"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("0518cd94b9003540f44ee7e5a517ca477a1fe33344da7bc84e257001d01c8148"));
    BOOST_CHECK_EQUAL(block.nVersion, 5964548);
    BOOST_CHECK_EQUAL(block.nTime, 1529670330);
    BOOST_CHECK_EQUAL(block.nBits, 0x1914b362);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_argon2) {
    CBlock block;

    // main chain #452010 argon block
    DataStream ds{ParseHex("04060000b5e03486f78d9a6fffc779bbd75e1b41f29888af1f8676cc9755fcd0fac09f63fb0b1a87eafdfa19c4682d990e4a61ba7b145af2f0488088732349ec45d64447d9d41a5b3f94351e71c71c990101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0403aae506ffffffff01002f6859000000001976a914556b84edccf1765069a516e125c80bed9bad2d6988ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::ARGON2);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("0xa27a2938d82324890aa2881cc572214c167b6e99289db06c5b239c8e2363c2d1"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("0x0000049cbe85045bb3fa6b1983e0728df9f47fcbd7a142b988b0f65385eb380f"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("639fc0fad0fc5597cc76861faf8898f2411b5ed7bb79c7ff6f9a8df78634e0b5"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("4744d645ec492373888048f0f25a147bba614a0e992d68c419fafdea871a0bfb"));
    BOOST_CHECK_EQUAL(block.nVersion, 1540);
    BOOST_CHECK_EQUAL(block.nTime, 1528485081);
    BOOST_CHECK_EQUAL(block.nBits, 0x1e35943f);
    BOOST_CHECK_EQUAL(block.nNonce, 2568800113);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_argon_aux)
{
    CBlock block;

    // main chain #1926883 block
    DataStream ds{ParseHex("04075b00cfe0f705a9079b5d2bb3b7c08d10177f031ef42bf6a8101a3fa7421eaacf9592e8a1b1e0e5279661e1059197fb1e8538d41ced3de3f491cfda0f0873f01389f8a8f3596637174c1d0000000001000000010000000000000000000000000000000000000000000000000000000000000000ffffffff3c03ab0d372cfabe6d6dfa7427daaff5a12ec5e9bd7d1d14e8d1f50a64b963473fbc8b96ed11f617667301000000000000007032702d7370622e78797affffffff060000000000000000266a24aa21a9ed92ac07c7b9e008effde0e4753064f2328b1c8cd3d0b488f4bf1bba8cb9338c3615d5cc05000000001976a914c3b8d290e654d85d28afc7baaf3ea680800693f788aceb759ad00200000017a914ac9558c9848dd836ed87ebae141602457f61701987fe6eb6fb020000001976a9140e202f946af5a0a5e6b8dfcc549d14b556271ebe88ac0200000000000000434104ffd03de44a6e11b9917f3a29f9443283d9871c9d743ef30d5eddcd37094b64d1b3d8090496b53256786bf5c82932ec23c3b74d9f05a6f95a8b5529352656664bac00000000000000002a6a287161bab8c911d8f74ef611d8081814cd20a2bf0d952f353c744cbb9b2a49a83b00000000e460001100000000d5665d88627c8575c69658c45db675f49d70e92deecc9f1c555c742842a6e25600000000000000000000000c5a20d673450d67b2b82595377f9429749972c254d1b2d4e197b0e1dda432c10777f46718bb73b05c2270f060b0394b4bd554bedc0e7ced5b531a9d59902612c05d4aedf359661f7a411d00055a910101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603e3661d0101ffffffff01db94ad2100000000232103cf1a6a963502a19c9e3b461c3c7e30256c94238336cbab17022252d4918bc6d3ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::ARGON2);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("fa7427daaff5a12ec5e9bd7d1d14e8d1f50a64b963473fbc8b96ed11f6176673"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("adfe59356c249ba4275996832dbbfbb42d4d7abff616c8288528139de80b9e2f"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("9295cfaa1e42a73f1a10a8f62bf41e037f17108dc0b7b32b5d9b07a905f7e0cf"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("f88913f073080fdacf91f4e33ded1cd438851efb979105e1619627e5e0b1a1e8"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlockPoWHash(block.GetAlgo()), uint256S("0000002afbdc1a3be5a1de4ebc3bc0b45180cf1c1e43aaf5b5243e0b28b77b45"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().GetHash(), uint256S("56e2a64228745c551c9fccee2de9709df475b65dc45896c675857c62885d66d5"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().hashPrevBlock, uint256S("f47707c132a4dde1b097e1d4b2d154c272997429947f379525b8b2670d4573d6"));
    BOOST_CHECK_EQUAL(block.nVersion, 5965572);
    BOOST_CHECK_EQUAL(block.nTime, 1717171112);
    BOOST_CHECK_EQUAL(block.nBits, 0x1d4c1737);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_x17)
{
    CBlock block;

    // main chain #452010 argon block
    DataStream ds{ParseHex("040800007a9781187b4d0e2a519f22d96cf110288361b5f2f941db9fcc1107d8bd586320aaa1a3a326b71945e89145fd0429374df6846a830fd8dee185ed94ddfe40506728d61a5b307b031cec0483650102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0c03ace5066f8fa602340000000000000001002f6859000000001976a914f330c69d5af89893d6e54108d3c9c6e51478d7fa88ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::X17);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("f006e64d65e95ae0ed495e77efc7276d5389f0269385683a1b57d602c9a36216"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("00000000027c8fae367ffcd61d1be7ffe1c15ac9b256162dc8a45be4965c3459"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("206358bdd80711cc9fdb41f9f2b561832810f16cd9229f512a0e4d7b1881977a"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("675040fedd94ed85e1ded80f836a84f64d372904fd4591e84519b726a3a3a1aa"));
    BOOST_CHECK_EQUAL(block.nVersion, 2052);
    BOOST_CHECK_EQUAL(block.nTime, 1528485416);
    BOOST_CHECK_EQUAL(block.nBits, 0x1c037b30);
    BOOST_CHECK_EQUAL(block.nNonce, 1703085292);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_x17_aux)
{
    CBlock block;

    // main chain #1726893 block
    DataStream ds{ParseHex("04095b002e20b9d1adc4a13731c313f29b50fdb3d987a1e90d60fda00520926df09b0a79d4efca677a0f9ae34050850ae8561e404f40e4a4f4c0689ae2076bbe8e8a6ec10008bd64cc364d1c0000000002000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4403258d34040008bd6408810013de000000007969696d7000fabe6d6df53bfc85f5478a721269f866387d249b25d8cde7ccaec206f03d0782defff4c90100000000000000000000000180e7bd020000000017a9145f72248225e157fb50a72f009b5efc2ad492f0c587000000000000000007fe9cac1ed146b6daa83ae4db46a79122a47f5407b525cd578d2e4900000000000000000000001800209f68cde4a876fb26ff63046f6c7eb391195d72f716bdcb977b5b0d7725fd3fa74ae24211fa443453d3b5ce84987dc93101161e0649b09eb4714eefaea19a39850008bd64f199231c56b55bd80101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603ad591a0101ffffffff019c8c571e00000000232102059ca8a16aba72e776d14fe5d6efa7bdd63fc0bc49c12d8024fe7ad12ab4f95aac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::X17);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("f53bfc85f5478a721269f866387d249b25d8cde7ccaec206f03d0782defff4c9"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("d4c1ac1298aef37b9b0b0c1a6bba04cbc57ce6080651b064eca9995d7d27ba5d"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("790a9bf06d922005a0fd600de9a187d9b3fd509bf213c33137a1c4add1b9202e"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("c16e8a8ebe6b07e29a68c0f4a4e4404f401e56e80a855040e39a0f7a67caefd4"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlockPoWHash(block.GetAlgo()), uint256S("0000000007fe9cac1ed146b6daa83ae4db46a79122a47f5407b525cd578d2e49"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().GetHash(), uint256S("2663c6cf79db85683dec51b38392ec47af064659d99f1773b38f94f925b5acd3"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().hashPrevBlock, uint256S("a73ffd25770d5b7b97cbbd16f7725d1991b37e6c6f0463ff26fb76a8e4cd689f"));
    BOOST_CHECK_EQUAL(block.nVersion, 5966084);
    BOOST_CHECK_EQUAL(block.nTime, 1690109952);
    BOOST_CHECK_EQUAL(block.nBits, 0x1c4d36cc);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_lyra2rev2)
{
    CBlock block;

    // main chain #452010 argon block
    DataStream ds{ParseHex("040a00001662a3c902d6571b3a68859326f089536d27c7ef775e49ede05ae9654de606f0ed5d4cb1e095102c983713dc7192a57fc2e996941f390f879fe9b2b24cc091dc44d61a5bb9ec011c6de7e1d90102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0c03ade50653b6a602090000000000000001de809d49000000001976a914f330c69d5af89893d6e54108d3c9c6e51478d7fa88ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::LYRA2REv2);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("ecab77573347b778c1426bb30558fa867795cb1d4b1239ef72769737db82a656"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("0000000001d5c23e9f1b967f7ec3c77da0ca5fab0d4fb8e18dfa9ce162617928"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("f006e64d65e95ae0ed495e77efc7276d5389f0269385683a1b57d602c9a36216"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("dc91c04cb2b2e99f870f391f9496e9c27fa59271dc1337982c1095e0b14c5ded"));
    BOOST_CHECK_EQUAL(block.nVersion, 2564);
    BOOST_CHECK_EQUAL(block.nTime, 1528485444);
    BOOST_CHECK_EQUAL(block.nBits, 0x1c01ecb9);
    BOOST_CHECK_EQUAL(block.nNonce, 3655460717);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_lyra2rev2_aux)
{
    CBlock block;

    // main chain #1726893 block
    DataStream ds{ParseHex("041b5b001a24bc46100813aa931a17b4efe46751f215dfcd338c123cc44132830f4121f0fb126bfe0cb8cac9f44f607c943276eeb5746ef504070152d817f51fc6e26ba24808bd64e4e5201c0000000002000000010000000000000000000000000000000000000000000000000000000000000000ffffffff440339a559040d09bd640881000167010000007969696d7000fabe6d6db9dd94faa0141e2b83cb13ea2e3b4c3fb5b521cc1279cb084fdb81c079b571fc0100000000000000000000000100a3e111000000001976a914b1bbc46534b951d332e1fb4ef3ef6e270823860f88ac00000000000000001ae50a4f35a9bb4410cf23aff52f45f61257985fc0433f2f69213c3a000000000000000000000404a324b147bf221d67f522e31792400556a97ffa08604082058dcee4c2944f22330813f52ba72cc6db4a6d0c143fe6aa980cd00eb515dbae9f85f9eb19e9d121d946c40d09bd647cc3001d181bf7280101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603af591a0101ffffffff01bf9c5b16000000002321028cb6a2323f10beb803c5b1a2ea562fcb3498774f4f061d6e4646ba62f8edcc93ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::LYRA2REv2);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("b9dd94faa0141e2b83cb13ea2e3b4c3fb5b521cc1279cb084fdb81c079b571fc"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("31b5dd5563d68c1db55a7797214df3bcba9e4bf534dbd52af23a28135ff778f9"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("f021410f833241c43c128c33cddf15f25167e4efb4171a93aa13081046bc241a"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("a26be2c61ff517d852010704f56e74b5ee7632947c604ff4c9cab80cfe6b12fb"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlockPoWHash(block.GetAlgo()), uint256S("000000001ae50a4f35a9bb4410cf23aff52f45f61257985fc0433f2f69213c3a"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().GetHash(), uint256S("e57a8ae55105754428f8c77de245c793d974dab193ac33c4a52a021d30c7562a"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().hashPrevBlock, uint256S("130833224f94c2e4ce8d0582406008fa7fa95605409217e322f5671d22bf47b1"));
    BOOST_CHECK_EQUAL(block.nVersion, 5970692);
    BOOST_CHECK_EQUAL(block.nTime, 1690110024);
    BOOST_CHECK_EQUAL(block.nBits, 0x1c20e5e4);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_yescrypt)
{
    CBlock block;

    // main chain #453107 argon block
    DataStream ds{ParseHex("04040000cf729a3c093ac6efc74bf1c27c700024be68283b3eb444c32efa208bef1b4d1b41fd5b761816edf18febc75fb2fb4050a79573737da29dfc63a4c1f1536a35b8aef01c5b8cf7421e800000260101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603f3e906010effffffff01002f685900000000232103cd0b1dd9f9c3966c1430af32bb4785d83091e764d289ab2c0ef3c51618be229dac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::YESCRYPT);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("bcfd8fa12d9f8ea5cb692195c5592a1d3dba92b466cc6cb482fb830d48eb7b6f"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("000006b9b14dd31a2d109a28570c993cb5fdef89f23d38563baa11bebdea7ce2"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("1b4d1bef8b20fa2ec344b43e3b2868be2400707cc2f14bc7efc63a093c9a72cf"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("b8356a53f1c1a463fc9da27d737395a75040fbb25fc7eb8ff1ed1618765bfd41"));
    BOOST_CHECK_EQUAL(block.nVersion, 1028);
    BOOST_CHECK_EQUAL(block.nTime, 1528623278);
    BOOST_CHECK_EQUAL(block.nBits, 0x1e42f78c);
    BOOST_CHECK_EQUAL(block.nNonce, 637534336);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));

}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_yescrypt_aux)
{
    CBlock block;

    // main chain #1926883 block
    DataStream ds{ParseHex("04055b008ba450d305298492c08ddd46b712c9a642cfb47b1f4abd9d17abf25f3189e48172842eba9d2bb8e77cce1bea9d18e31ee405d79b8ce8c05f51d32a8e59b7b9559d548a653192021e0000000002000000010000000000000000000000000000000000000000000000000000000000000000ffffffff44038d6636049d548a650881000110000000007969696d7000fabe6d6d838c03ba9b85e3ee25b9507369b380faf572a6c73a2cb61ebbf02d94b959f4b00100000000000000000000000100ba1dd2050000001976a91460fc6dad3e2eb3bf7e9dfe83d397913dcb2019aa88ac00000000000001ab93f986cc610891e8daf4545e2d66ac74e9e8a9377a9a6843ff51e16600000000000000000000000a5a20532f2e1ca9e8fc4a9d612c7595628dffa525d44b169a89a0fe7a619294bdad8e0ae516cfb2e60d83de7e671ab0dd5eafef89eca936088b06f91a390c2e03768d9d548a655a0a161d190200800101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff060337e01b0101ffffffff01aefef718000000002321039014e5e38a7135de169bede35ef3fab9fe482b950bc362beed574dbbd3a4e9feac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::YESCRYPT);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("838c03ba9b85e3ee25b9507369b380faf572a6c73a2cb61ebbf02d94b959f4b0"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("5146100db8fe57c17d454a38b4377a56dc18fedee368c42ae8d71fe7ab1f5bc9"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("81e489315ff2ab179dbd4a1f7bb4cf42a6c912b746dd8dc092842905d350a48b"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("55b9b7598e2ad3515fc0e88c9bd705e41ee3189dea1bce7ce7b82b9dba2e8472"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlockPoWHash(block.GetAlgo()), uint256S("000001ab93f986cc610891e8daf4545e2d66ac74e9e8a9377a9a6843ff51e166"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().GetHash(), uint256S("c7a60b123c829884a4086220a9cea3c448349a29b73810b5beffd1cdaff82ab6"));
    BOOST_CHECK_EQUAL(block.auxpow->getParentBlock().hashPrevBlock, uint256S("8eadbd9492617afea0899a164bd425a5ff8d6295752c619d4afce8a91c2e2f53"));
    BOOST_CHECK_EQUAL(block.nVersion, 5965060);
    BOOST_CHECK_EQUAL(block.nTime, 1703564445);
    BOOST_CHECK_EQUAL(block.nBits, 0x1e029231);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK(CheckAuxPowProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_equihash)
{
    CBlock block;

    // main chain #453108 block
    DataStream ds{ParseHex("040c00006f7beb480d83fb82b46ccc66b492ba3d1d2a59c5952169cba58e9f2da18ffdbcb26d192436490db8d39bf12f3474a25e256fc304085745cb103e83d91c18980d00000000000000000000000000000000000000000000000000000000000000003df41c5b6dc0031feb01000000000000000000000000000000000000000000000000000000000000fd400500d2a5508c048311b15c5386c43f3f0e1bc61bb23235f8543f4ad53631a47639152d65476a728e3ec3c80a1094e57cd242b9c7af020a89f99b11038b9bd8820bb0d1064089d339c43063adec4579e7781dde04a8090bde2ef1cf60bbc147c31eed706c7dfc227ae9a8415abcbc0f522a61523964cbc577f1428409fff2f70fa3514f9c510006e3c1230e0b44116670b6ba5904124ffe96584d9b2b4aaab4eeb04996b9c50bd9ce4d037c70e581a6a09b61f840de3955b491e2a692f15414d0e5e70ad8bb01dbd0a1c109766cfe9b40b852c008cfb2b3468b4d18971a41b5da5c85b54a4a0eb6a563c6fce84e2d3e59eefea83d825efcbf9413be70e50da167a9b1ffe455ff8c8430cbb050c195f49f87a320bef312f4d7b60be27f47b310ed10b6f5affa51ac1d55ae60838d3783ed9d9baf866cf247adcbbf8d5724420c168b15afd93c6cd35250bcca2605c576966e00f29b89168c949d8dc91497a0c405aa9f44fa0c9a07e505bf188716093b6031138a4540397d7aefc2a20a4f1ffef07a418fff57225496bd359b4dc65dc95830bcac24fc5853b582f0480dd3ede9123b801212ce0246b2d500e8da05f4d9a5b47776977d7b4e3186cc029772952b02f80a2df6807cbc7932b4536fc79eb71c4e67ac6089e3291e6622d986287dc9ea543f80bb328df49ebcaa3407f819d44340b6418a5cfebb8b5903b784aea14253f330af819523a682849bcb8c6a37239139474f9b085b2bf969b9a95c9b02f80918b75e0561746d9598fba5bf0b57d8bc589b0622eab6414209a0d8f7ee90c16b0964710e935569a9ea501eb6d90b7a90d8672365df94a9c507c667ac1eb37df9f8890e8cba6c5e86adcd2d6be187caa49bcccae82e45a621fe07127bd9c9193c9c9b3f84e0847b9b217e25184ede3aa1a4d8a135d59447aacdd8c83b90193f494d0447b47e5f08e4578d93d0ef26fddfff11d61aa52128af426aa14b855dc86698eac25384b2d5399747a704e3fff6d88df0abcf084c9f026bbc23605a3cc23f20baa1235b5d3eeb7a4cf4591c5a5952f4eaff8ae21dfb0e75b99cc5a15d3203a989db3ba121a8c9f84660ac9f1c431996fd116e782c93cfecaa835598aa30343e4d407bf340addf68d863b1d8b9b328733f84a536f79f3a69d2f367cf8c68ab38e6609a55f59c25f50550f750a0d88a31b5e8e16730509ac92fe14e1ffc0ded398f899f02454fe927fcc050761a3e9dd2f57e08a351cd1943440667c690c3017c23ad99d9ba372c3b461f762e0f52a179bc868017f3d566225833bc920fb074e0bbcd7075629782956452a3890e46fdb8b02a8187c08de923ed91adb990867973be92fffd62a84639ae5b1fa0062b0355e493bef59a99c3c299d53c5d6c5e6324f7953bf996c95568d431b659fd1a4429060779abe862ab01260d7368fca18a21a9a6f16a93295f49c6481d185d24f116c002711c95e9954f5ed311cfefd04b509582c32ae18b31e72128bff73a2e0e48ed9660e0eee885a0b0685869fa1e064a2913f9b708db21b789cab041a1776480623f82fd7bc0f14ed71a5c9bf152a49e9b30e41a516b5cf0b696603ba6de0fcd3fa7f95659e176d4326d83be350cb629ef5abb1cedfe536fa2afe925c555bcfbeeb19f1876dc47ef07898ecfa068cf87ea4f011aa8563eb4959766a6fc4055fb3d669406d7f2f84528e6512d7e1a621f95641e2cc1d6b69f2ff710bb278079bf10523aed38bd9326cfd7f3eaca50bd66f798694d6559cf3e87dd39560b5a71276e62188d5242d2cfec44e33609b3fa75031c5e5689a9d7e4acfe3b65c847f7c16d956b76d5fd0d126630ebce4904d1c60394d15642f1f086d4bf6f278741ef36e459d334ae96eb0471c8235b619fdd190101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603f4e9060122ffffffff01002f6859000000002321034671166383d5ad9cec8bfb14df8d84e3561dc320285043d070ab049dea593cdeac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::EQUIHASH);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("0000566e60a1702289dd8044e6f5ee2776dcfcaca52c68e5ebc83754cb3d5ff1"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("0000566e60a1702289dd8044e6f5ee2776dcfcaca52c68e5ebc83754cb3d5ff1"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("bcfd8fa12d9f8ea5cb692195c5592a1d3dba92b466cc6cb482fb830d48eb7b6f"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("0d98181cd9833e10cb45570804c36f255ea274342ff19bd3b80d493624196db2"));
    BOOST_CHECK_EQUAL(block.nVersion, 3076);
    BOOST_CHECK_EQUAL(block.nTime, 1528624189);
    BOOST_CHECK_EQUAL(block.nBits, 0x1f03c06d);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK_EQUAL(true, CheckProofOfWork(block));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_cryptonight)
{
    CBlock block;

    // main chain #453108 block
    DataStream ds{ParseHex("040e000049be550e7443f0c95daab964a765548e9c7018866034a77934bcb949b42cbaa03580446a49fc2ab824815c0cad797c5db8226632e69562d73729bfb3f483bb7a4a8aec5b0c4b011e6e1400000301000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603929f080118ffffffff01eeef514000000000232102d0ce237a2166d3561f5189c5f81a5badf2cd1290401653f443acb12ebdf932a2ac000000000100000002fa4dd24845b2cb419156e0db359b3aef13c4563a52cfdaebe5854d24d0ee5de0000000006b483045022100e50a3515f9c697de83b363d453e598b4a87b5a0610a41c0316cfde9bf58a7bae02202adac05274e0d9a3351ff546f946cc0d72ab1c9482c88297205ee8ef2de0033001210369f01e279797e351be627663980e825d875645ec3f88cca83bf148f5485f152dffffffff48a4f565b7b0b6c62268db9d7993c94aab5c450e3bd1b5782d937cfba2d946d6000000006b483045022100c3be6219786f957c75813627ce7e346c52481f047bec107d0dcd6db319e1a1960220407021a855b44682f6cb80a5c5ec96b2b590d8f18667be39c614e2d5559a827901210369f01e279797e351be627663980e825d875645ec3f88cca83bf148f5485f152dffffffff0138f84868000000001976a91448e806902ce559c8c410435f52e176865d831cbd88ac000000000100000001ea76640f72fa50121fda67e5307df15b343e96bc8b93b0c92195b47759866d33000000006a47304402201b15ed94711f77dcab48888bf5191d0ad06716c07ae35a883d9f8f5773603b93022079d96756626fbcd410921f6e9f37ed8a1fa056966d86384c4da2961464441230012103e9882c39b2a2b847f516234d60b2f17d7ccb1da8e4bef8b3ac21c09c76de8374ffffffff0140420f00000000001976a9142895a5675dc1052297b6f65a58f74684a78a65d788ac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::CRYPTONIGHT);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("52c4078b3742ea8eab5ba4ee0dbce04970f748cffa9e53c52c65b38af446bea4"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("000000cde85c1dc4d693d6db1c983523e36e774aaaea94d69ba0d785ed4e3d0a"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("a0ba2cb449b9bc3479a734608618709c8e5465a764b9aa5dc9f043740e55be49"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("7abb83f4b3bf2937d76295e6326622b85d7c79ad0c5c8124b82afc496a448035"));
    BOOST_CHECK_EQUAL(block.nVersion, 3588);
    BOOST_CHECK_EQUAL(block.nTime, 1542228554);
    BOOST_CHECK_EQUAL(block.nBits, 0x1e014b0c);
    BOOST_CHECK_EQUAL(block.nNonce, 5230);
    BOOST_CHECK_EQUAL(block.vtx.size(), 3);

    BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo()));
}

BOOST_AUTO_TEST_CASE(validate_multialgo_block_cryptonight_aux)
{
    CBlock block;

    // main chain #453108 block
    DataStream ds{ParseHex("040f5b003bd4df591e1216bff76b63ff6022d9dd43e1483b8a7e5974426fa30eeb4ecf34f695085384086fff2f607494b436b890ae8e94301a1503bc5a9ec83e1748a9b014a5395bd148081e000000008102c18f6201ff858f6201bbd1bceb877e02a5134b04552250db84dd40b93c37ad466f1bf342840131a6267d0cba892eba7c4f010891d1d14f88998e1274dbbc0d39b5cf7144a8d7a46446d7fa7c7ba5ef692934022cfabe6d6d6baf91c1639bbec627bfaee992e85077215f4804640616c0bd4d1f64885cf47d010000000000000004e5991ea8ddf8126a79cc5ad5a22d6c7b5c5bc7f46ea5905faea67d3143a14c0144f6cfbcdfea6daacd30b01977402a9ed0fbbe71c446c43f4d4e89a08fa35d570000000000000000004c0707a4cbe6d905e5434671fb8ab1093a3a63158659d1e185b3cafe2c084cfc8891a6dde6aadee20bd2cc4ce08a590e247b163af4b00902b08f7e627cd46d89f33cc637f70163a098357c2c030101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0603f127070101ffffffff019b9ce35300000000232103e68fd5c2da10fea0da8b658d6a0daf61cd1974e95ee4cae290ac076dd09dea4dac00000000")};
    ds >> TX_NO_WITNESS(block);

    BOOST_CHECK_EQUAL(block.GetAlgo(), Algo::CRYPTONIGHT);
    BOOST_CHECK_EQUAL(block.GetHash(), uint256S("6baf91c1639bbec627bfaee992e85077215f4804640616c0bd4d1f64885cf47d"));
    BOOST_CHECK_EQUAL(block.GetPoWHash(), uint256S("b4eec3977034d3ec872a90efded4e9388ab3315385ea946673a2ebaf21d74d9f"));
    BOOST_CHECK_EQUAL(block.hashPrevBlock, uint256S("34cf4eeb0ea36f4274597e8a3b48e143ddd92260ff636bf7bf16121e59dfd43b"));
    BOOST_CHECK_EQUAL(block.hashMerkleRoot, uint256S("b0a948173ec89e5abc03151a30948eae90b836b49474602fff6f0884530895f6"));
    BOOST_CHECK_EQUAL(block.nVersion, 5967620);
    BOOST_CHECK_EQUAL(block.nTime, 1530504468);
    BOOST_CHECK_EQUAL(block.nBits, 0x1e0848d1);
    BOOST_CHECK_EQUAL(block.nNonce, 0);
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);

    BOOST_CHECK_EQUAL(true, CheckProofOfWork(block));
}

BOOST_AUTO_TEST_SUITE_END()
