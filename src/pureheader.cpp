#include "pureheader.h"
#include "util.h"
#include "hash.h"

uint256 CPureBlockHeader::GetHash() const
{
  return Hash(BEGIN(nVersion), END(nNonce));
}
