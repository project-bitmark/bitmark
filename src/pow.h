#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "uint256.h"
#include "core.h"

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);

/** Check whether the block's equihash solultion is valid, in the case of an equihash algo */
bool CheckEquihashSolution(const CPureBlockHeader *pblock, const CChainParams& params);

#endif
