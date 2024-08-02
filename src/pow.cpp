// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <uint256.h>
#include <logging.h>
#include <streams.h>
#include <crypto/equihash/equihash.h>
#include <node/protocol_version.h>
#include <serialize.h>
#include <util/bignum.h>

uint32_t static DarkGravityWave(const CBlockIndex* pindexLast, Algo algo)
{
    /* current difficulty formula, DASH - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 25;
    int64_t PastBlocksMax = 25; // We have same max and min, just using same variables from old code
    int64_t CountBlocks = 0;
    CBigNum PastDifficultyAverage;
    CBigNum PastDifficultyAveragePrev;
    CBigNum LastDifficultyAlgo;
    int64_t time_since_last_algo = -1;
    int64_t LastBlockTimeOtherAlgos = 0;
    unsigned int algoWeight = GetAlgoWeight(algo);

    int lastInRow = 0;          // starting from last block from algo to first occurence of another algo
    bool lastInRowDone = false; // once another algo is found, stop the count

    int nInRow = 0;          // consecutive sequence of blocks from algo within the 25 block period
    bool nInRowDone = false; // if an island of 9 or more is found, then stop the count

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit(algo).GetCompact();
    }

    for (int i = 0; BlockReading; i++) {
        if (!BlockReading->OnFork()) { // last block before fork
            if (LastBlockTime > 0) {
                nActualTimespan = (LastBlockTime - BlockReading->GetBlockTime());
            }
            if (LastBlockTimeOtherAlgos > 0 && time_since_last_algo == -1) {
                time_since_last_algo = LastBlockTimeOtherAlgos - BlockReading->GetBlockTime();
            }
            CountBlocks++;
            if (nInRow < 9) {
                nInRow = 0;
            } else {
                nInRowDone = true;
            }
            break;
        }

        if (!LastBlockTimeOtherAlgos) {
            LastBlockTimeOtherAlgos = BlockReading->GetMedianTimePast();
        }

        Algo block_algo = BlockReading->GetAlgo();
        if (block_algo != algo) { // Only consider blocks from same algo
            BlockReading = BlockReading->pprev;
            if (CountBlocks) lastInRowDone = true;
            if (nInRow < 9) {
                nInRow = 0;
            } else {
                nInRowDone = true;
            }
            continue;
        }
        if (!CountBlocks) LastDifficultyAlgo.SetCompact(BlockReading->nBits);

        CountBlocks++;
        if (!nInRowDone) nInRow++;
        if (!lastInRowDone) lastInRow++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
                if (LastBlockTimeOtherAlgos > 0) time_since_last_algo = LastBlockTimeOtherAlgos - BlockReading->GetMedianTimePast();
                LastBlockTime = BlockReading->GetMedianTimePast();
                LogDebug(BCLog::VALIDATION, "block time final = %d\n", LastBlockTime);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * (CountBlocks - 1)) + (CBigNum().SetCompact(BlockReading->nBits))) / CountBlocks;
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            if (LastBlockTime > 0) {
                nActualTimespan = (LastBlockTime - BlockReading->GetMedianTimePast());
            }
            break;
        }
        if (CountBlocks >= PastBlocksMax) {
            if (LastBlockTime > 0) {
                LogDebug(BCLog::VALIDATION, "block time initial %d\n", BlockReading->GetMedianTimePast());
                nActualTimespan = (LastBlockTime - BlockReading->GetMedianTimePast());
            }
            break;
        }

        BlockReading = BlockReading->pprev;
    }

    int pastInRow = 0; // if not done counting, count the past blocks in row with algo starting at the boundary and going back
    if ((nInRow && !nInRowDone || lastInRow && !lastInRowDone) && BlockReading) {
        LogDebug(BCLog::VALIDATION, "nInRow = %d and not done\n", nInRow);
        const CBlockIndex* BlockPast = BlockReading->pprev;
        while (BlockPast) {
            if (GetAlgo(BlockPast->nVersion) != algo || !BlockPast->OnFork()) {
                break;
            }
            pastInRow++;
            BlockPast = BlockPast->pprev;
        }
        if (!lastInRowDone) lastInRow += pastInRow;
    }

    CBigNum bnNew;
    int lastInRowMod = lastInRow % 9;
    LogDebug(BCLog::VALIDATION, "nInRow = %d lastInRow=%d\n", nInRow, lastInRow);
    bool justHadSurge = nInRow >= 9 || nInRow && pastInRow && (nInRow + pastInRow) >= 9 && pastInRow % 9 != 0;
    if (justHadSurge || time_since_last_algo > 9600) {
        LogDebug(BCLog::VALIDATION, "bnNew = LastDifficultyAlgo\n");
        bnNew = LastDifficultyAlgo;
    } else {
        bnNew = PastDifficultyAverage;
    }
    int64_t _nTargetTimespan = (CountBlocks - 1) * Params().GetConsensus().DGWtimespan; // 16 min target

    int64_t smultiplier = 1;
    bool smultiply = false;
    if (time_since_last_algo > 9600) { // 160 min for special retarget
        smultiplier = time_since_last_algo / 9600;
        LogPrintf("special retarget for algo %d with time_since_last_algo = %d (height %d), smultiplier %d\n", algo, time_since_last_algo, pindexLast->nHeight, smultiplier);
        nActualTimespan = 10 * smultiplier * _nTargetTimespan;
        smultiply = true;
    }

    if (lastInRow >= 9 && !lastInRowMod)
        LogDebug(BCLog::VALIDATION, "activate surge protector\n");

    if (nActualTimespan < _nTargetTimespan / 3 || lastInRow >= 9 && !lastInRowMod)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = smultiplier * _nTargetTimespan * 3;

    if (CountBlocks >= PastBlocksMin) {
        if (lastInRow >= 9 && !lastInRowMod) {
            bnNew /= 3;
        } else if (!justHadSurge || smultiply && CBlockIndex::IsSuperMajorityVariant12(4, true, pindexLast, 950, 1000)) {
            bnNew *= nActualTimespan;
            bnNew /= _nTargetTimespan;
        }
    } else if (CountBlocks == 1) { // first block of algo for fork
        LogDebug(BCLog::VALIDATION, "CountBlocks = %d\n", CountBlocks);
        LogDebug(BCLog::VALIDATION, "setting nBits to keep continuity of scrypt chain\n");
        LogDebug(BCLog::VALIDATION, "scaling wrt block at height %u algo %d\n", BlockReading->nHeight, algo);

        unsigned int weightScrypt = GetAlgoWeight(Algo::SCRYPT);
        if (algo == Algo::SCRYPT || algo == Algo::SHA256D) {
            bnNew.SetCompact(BlockReading->nBits); // preserve continuity of chain diff for scrypt and sha256d
            bnNew *= algoWeight;
            bnNew /= (8 * weightScrypt);
        } else {
            if (Params().GetConsensus().fPowAllowMinDifficultyBlocks) {
                bnNew.SetCompact(BlockReading->nBits);
            } else {
                bnNew.SetCompact(0x1d00ffff); // for newer algos, use min diff times 128, weighted
            }
            bnNew *= algoWeight;
            bnNew /= 128;
        }
        if (smultiply) bnNew *= smultiplier * 3;
    } else {
        if (smultiply) bnNew *= smultiplier * 3;
        if (lastInRow >= 9 && !lastInRowMod) bnNew /= 3;
    }

    if (bnNew > Params().ProofOfWorkLimit(algo)) {
        bnNew = Params().ProofOfWorkLimit(algo);
    }

    LogDebug(BCLog::VALIDATION, "DarkGravityWave RETARGET algo %d\n", algo);
    LogDebug(BCLog::VALIDATION, "_nTargetTimespan = %d    nActualTimespan = %d\n", _nTargetTimespan, nActualTimespan);
    LogDebug(BCLog::VALIDATION, "Before: %08x  %lu\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString());
    LogDebug(BCLog::VALIDATION, "BlockReading: %08x %lu\n", BlockReading->nBits, CBigNum().SetCompact(BlockReading->nBits).getuint256().ToString());
    LogDebug(BCLog::VALIDATION, "Avg from past %d: %08x %lu\n", CountBlocks, PastDifficultyAverage.GetCompact(), PastDifficultyAverage.getuint256().ToString());
    LogDebug(BCLog::VALIDATION, "After:  %08x  %lu\n", bnNew.GetCompact(), bnNew.getuint256().ToString());

    return bnNew.GetCompact();
}


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, Algo algo)
{
    Consensus::Params params = Params().GetConsensus();

    if (params.fPowNoRetargeting)
        return UintToArith256(params.powLimit).GetCompact();

    if (!CBlockIndex::IsSuperMajority(4, pindexLast, 75, 100)) {
        unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

        // Genesis block
        if (pindexLast == NULL) {
            return nProofOfWorkLimit;
        }

        int64_t nInterval = params.DifficultyAdjustmentInterval();
        // Only change once per interval
        if ((pindexLast->nHeight + 1) % nInterval != 0)
        {
            if (pindexLast->nHeight == 0) {
                return nProofOfWorkLimit;
            }
            return pindexLast->nBits;
        }

        // Go back by what we want to be a days worth of blocks
        const CBlockIndex* pindexFirst = pindexLast;
        for (int i = 0; pindexFirst && i < nInterval - 1; i++)
            pindexFirst = pindexFirst->pprev;
        assert(pindexFirst);

        // Limit adjustment step
        int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
        LogDebug(BCLog::VALIDATION, "  nActualTimespan = %d  before bounds\n", nActualTimespan);

        if (nActualTimespan < params.nPowTargetTimespan / 4)
            nActualTimespan = params.nPowTargetTimespan / 4;
        if (nActualTimespan > params.nPowTargetTimespan * 4)
            nActualTimespan = params.nPowTargetTimespan * 4;

        // Retarget
        CBigNum bnNew;
        bnNew.SetCompact(pindexLast->nBits);
        bnNew *= nActualTimespan;
        bnNew /= params.nPowTargetTimespan;

        CBigNum bnPowLimit(params.powLimit);

        if (bnNew > bnPowLimit)
            bnNew = bnPowLimit;

        /// debug print
        LogDebug(BCLog::VALIDATION,"GetNextWorkRequired RETARGET\n");
        LogDebug(BCLog::VALIDATION,"nTargetTimespan = %d    nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
        LogDebug(BCLog::VALIDATION,"Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString());
        LogDebug(BCLog::VALIDATION,"After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString());

        return bnNew.GetCompact();
    } else {
        // Post 8mPoW fork
        return DarkGravityWave(pindexLast, algo);
    }
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime)
{
    Consensus::Params params = Params().GetConsensus();

    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    LogDebug(BCLog::VALIDATION, "GetNextWorkRequired RETARGET\n");
    LogDebug(BCLog::VALIDATION,"nTargetTimespan = %d    nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
    LogDebug(BCLog::VALIDATION,"Before: %08x  %s\n", pindexLast->nBits, arith_uint256().SetCompact(pindexLast->nBits).ToString());
    LogDebug(BCLog::VALIDATION,"After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

// Check that on difficulty adjustments, the new difficulty does not increase
// or decrease beyond the permitted limits.
bool PermittedDifficultyTransition(int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    Consensus::Params params = Params().GetConsensus();

    if (params.fPowAllowMinDifficultyBlocks)
        return true;

    // asume true after for since lack of CBlockIndex at this function, we can't calculate if diff transition is permitted.
    if (height > 450866) {
        return true;
    }

    if (height % params.DifficultyAdjustmentInterval() == 0) {
        int64_t smallest_timespan = params.nPowTargetTimespan/4;
        int64_t largest_timespan = params.nPowTargetTimespan*4;

        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);

        // Calculate the largest difficulty value possible:
        arith_uint256 largest_difficulty_target;
        largest_difficulty_target.SetCompact(old_nbits);
        largest_difficulty_target *= largest_timespan;
        largest_difficulty_target /= params.nPowTargetTimespan;

        if (largest_difficulty_target > pow_limit) {
            largest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 maximum_new_target;
        maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());

        if (maximum_new_target < observed_new_target) return false;

        // Calculate the smallest difficulty value possible:
        arith_uint256 smallest_difficulty_target;
        smallest_difficulty_target.SetCompact(old_nbits);
        smallest_difficulty_target *= smallest_timespan;
        smallest_difficulty_target /= params.nPowTargetTimespan;

        if (smallest_difficulty_target > pow_limit) {
            smallest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 minimum_new_target;
        minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
        if (minimum_new_target > observed_new_target) return false;
    } else if (old_nbits != new_nbits) {
        return false;
    }
    return true;
}

bool CheckEquihashSolution(const CPureBlockHeader* pblock)
{
    Consensus::Params params = Params().GetConsensus();

    if (pblock->nSolution.size() > 1) {
        // LogPrintf("check equihash solution hashprevblock=%s solution size = %d part = %x %x\n",pblock->hashPrevBlock.GetHex().c_str(),pblock->nSolution.size(),pblock->nSolution[0],pblock->nSolution[1]);
    } else {
        // LogPrintf("check equihash solution with solution size = %d\n",pblock->nSolution.size());
    }

    unsigned int n = params.EquihashN();
    unsigned int k = params.EquihashK();

    // Hash state
    crypto_generichash_blake2b_state state;
    EhInitialiseState(n, k, state);

    // I = the block header minus nonce and solution.
    CEquihashInput I{*pblock};
    // I||V
    DataStream ss;
    ss << I;
    ss << pblock->nNonce256;

    /*LogPrintf("checkES ss (%lu) = ",ss.size());
    for (int i=0; i<ss.size(); i++) {
      LogPrintf("%02x",*((unsigned char *)&ss[0]+i));
    }

    LogPrintf("\n");
    LogPrintf("checkES nSolution (%lu) = ",(pblock->nSolution).size());
    for (int i=0; i<(pblock->nSolution).size(); i++) {
      LogPrintf("%02x",*((unsigned char *)&(pblock->nSolution)[0]+i));
    }
    LogPrintf("\n");*/

    // H(I||V||...
    crypto_generichash_blake2b_update(&state, (unsigned char*)&ss[0], ss.size());

    bool isValid;
    EhIsValidSolution(n, k, state, pblock->nSolution, isValid);
    if (!isValid)
        return error("CheckEquihashSolution(): invalid solution");

    return true;
}

bool CheckProofOfWork(const CBlockHeader& block)
{
    if (block.IsAuxpow()) {
        return CheckAuxPowProofOfWork(block);
    }

    if (block.GetAlgo() == Algo::EQUIHASH && !CheckEquihashSolution(&block)) {
        return false;
    }

    return CheckProofOfWork(block.GetPoWHash(), block.nBits, block.GetAlgo());
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, Algo algo)
{
    Consensus::Params params = Params().GetConsensus();

    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit) * GetAlgoWeight(algo))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget) {
        return false;
    }

    return true;
}


bool CheckAuxPowProofOfWork(const CBlockHeader& block)
{
    Consensus::Params params = Params().GetConsensus();
    Algo algo = block.GetAlgo();
    /*if (block.auxpow || block.IsAuxpow()) {
      LogPrintf("checking auxpowproofofwork for algo %d\n",algo);
      LogPrintf("chain id : %d\n",block.GetChainId());
      }*/

    if (block.nVersion > 3 && block.IsAuxpow() && params.fStrictChainId && block.GetChainId() != params.nAuxpowChainId) {
        LogPrintf("auxpow err 1\n");
        return error("%s : block does not have our chain ID"
                     " (got %d, expected %d, full nVersion %d)",
                     __func__,
                     block.GetChainId(),
                     params.nAuxpowChainId,
                     block.nVersion);
    }

    if (!block.auxpow) {
        if (block.IsAuxpow()) {
            LogPrintf("auxpow err 2\n");
            return error("%s : no auxpow on block with auxpow version",
                         __func__);
        }

        if (!CheckProofOfWork(block.GetPoWHash(algo), block.nBits, block.GetAlgo())) {
            LogPrintf("auxpow err 3\n");
            return error("%s : non-AUX proof of work failed", __func__);
        }

        return true;
    }

    if (!block.IsAuxpow()) {
        LogPrintf("auxpow err 4\n");
        return error("%s : auxpow on block with non-auxpow version", __func__);
    }

    if (!block.auxpow->check(block.GetHash(), block.GetChainId())) {
        LogPrintf("auxpow err 5\n");
        return error("%s : AUX POW is not valid", __func__);
    }

    if (false) {  // TODO: fDebug
        arith_uint256 bnTarget;
        bnTarget.SetCompact(block.nBits);
        uint256 target = ArithToUint256(bnTarget);

        LogPrintf("DEBUG: proof-of-work submitted  \n  parent-PoWhash: %s\n  target: %s  bits: %08x \n",
                  block.auxpow->getParentBlockPoWHash(algo).ToString().c_str(),
                  target.GetHex().c_str(),
                  bnTarget.GetCompact());
    }

    if (block.GetAlgo() == Algo::EQUIHASH && !CheckEquihashSolution(&(block.auxpow->parentBlock))) {
        return error("%s : AUX equihash solution failed", __func__);
    }

    if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(algo), block.nBits, block.GetAlgo())) {
        return error("%s : AUX proof of work failed", __func__);
    }

    return true;
}
