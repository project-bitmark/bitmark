#include "pow.h"
#include "bignum.h"
#include "chainparams.h"
#include "util.h"

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    // Check range
    if (bnTarget <= 0 || bnTarget > Params().ProofOfWorkLimit()) {
      return error("CheckProofOfWork() : nBits below minimum work");
    }

    // Check proof of work matches claimed amount
    if (hash > bnTarget.getuint256()) {
      return error("CheckProofOfWork() : hash doesn't match nBits");
    }

    return true;
}
