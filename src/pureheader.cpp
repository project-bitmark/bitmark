#include "pureheader.h"
#include "util.h"
#include "hash.h"
#include "core.h"

uint256 CPureBlockHeader::GetHash() const
{
  if (GetAlgo()==ALGO_EQUIHASH) {
    return GetHashE();
  }
  return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CPureBlockHeader::GetHashE() const
{
  unsigned char input[1487];
  memcpy(input,BEGIN(nVersion),4);
  memcpy(input+4,BEGIN(hashPrevBlock),32);
  memcpy(input+36,BEGIN(hashMerkleRoot),32);
  memcpy(input+68,BEGIN(hashReserved),32);
  memcpy(input+100,BEGIN(nTime),4);
  memcpy(input+104,BEGIN(nBits),4);
  memcpy(input+108,BEGIN(nNonce256),32);
  if (nSolution.size()==1344) {
    input[140] = 0xfd;
    input[141] = 0x40;
    input[142] = 0x05;
    memcpy(input+143,BEGIN(nSolution[0]),1344);
  }
  else {
    LogPrintf("nSolution size %lu/n",nSolution.size());
    return Hash((unsigned char *)input,(unsigned char *)input+141);
  }
  return Hash((unsigned char *)input,(unsigned char *)input+1487);
}
