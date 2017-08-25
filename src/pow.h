#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "uint256.h"

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);

#endif
