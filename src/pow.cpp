#include "pow.h"
#include "bignum.h"
#include "chainparams.h"
#include "util.h"
#include "equihash.h"

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
      return error("CheckProofOfWork() : hash doesn't match nBits (hash is %s, nbits is %s",hash.GetHex().c_str(),bnTarget.getuint256().GetHex().c_str());
    }

    return true;
}

bool CheckEquihashSolution(const CBlockHeader *pblock, const CChainParams& params)
{

  if (pblock->nSolution.size()>1) {
    LogPrintf("check equihash solution hashprevblock=%s solution size = %d part = %x %x\n",pblock->hashPrevBlock.GetHex().c_str(),pblock->nSolution.size(),pblock->nSolution[0],pblock->nSolution[1]);
  }
  else {
    LogPrintf("check equihash solution with solution size = %d\n",pblock->nSolution.size());
  }
  
    unsigned int n = params.EquihashN();
    unsigned int k = params.EquihashK();

    // Hash state
    crypto_generichash_blake2b_state state;
    EhInitialiseState(n, k, state);

    // I = the block header minus nonce and solution.
    CEquihashInput I{*pblock};
    // I||V
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << I;
    ss << pblock->nNonce256;

    // H(I||V||...
    crypto_generichash_blake2b_update(&state, (unsigned char*)&ss[0], ss.size());

    bool isValid;
    EhIsValidSolution(n, k, state, pblock->nSolution, isValid);
    if (!isValid)
        return error("CheckEquihashSolution(): invalid solution");

    LogPrintf("Good equihash solution\n");
    
    return true;
}
