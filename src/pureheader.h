#ifndef BITCOIN_PUREHEADER_H
#define BITCOIN_PUREHEADER_H

#include "serialize.h"
#include "uint256.h"
#include "util.h"
#include "scrypt.h"
#include "hash.h"
#include "bignum.h"

const int NUM_ALGOS = 8;

enum {
  ALGO_SCRYPT = 0,
  ALGO_SHA256D = 1,
  ALGO_YESCRYPT = 2,
  ALGO_ARGON2 = 3,
  ALGO_X17 = 4,
  ALGO_LYRA2REv2 = 5,
  ALGO_EQUIHASH = 6,
  ALGO_CRYPTONIGHT = 7
};

/* Use the rightmost 8 bits for standard version number, 9th bit for merge mining, 10-12 th bits for POW algo, 13 th bit for update scaling factor flag */
enum
  {
    BLOCK_VERSION_AUXPOW = (1 << 8),
    BLOCK_VERSION_ALGO = (7 << 9),
    BLOCK_VERSION_SCRYPT = (0 << 9),
    BLOCK_VERSION_SHA256D = (1 << 9),
    BLOCK_VERSION_YESCRYPT = (2 << 9),
    BLOCK_VERSION_ARGON2 = (3 << 9),
    BLOCK_VERSION_X17 = (4 << 9),
    BLOCK_VERSION_LYRA2REv2 = (5 << 9),
    BLOCK_VERSION_EQUIHASH = (6 << 9),
    BLOCK_VERSION_CRYPTONIGHT = (7 << 9),
    BLOCK_VERSION_UPDATE_SSF = (1 << 12),
    BLOCK_VERSION_CHAIN = (1 << 16)
  };

class CPureBlockHeader { // Needed to resolve circular dependecies with CAuxPow in CBlockHeader
 public:
  static const int CURRENT_VERSION=4;
  int nVersion;
  uint256 hashPrevBlock;
  uint256 hashMerkleRoot;
  unsigned int nTime;
  unsigned int nBits;
  unsigned int nNonce;
  uint256 nNonce256;
  std::vector<unsigned char> nSolution;
  uint256 hashReserved;
  bool isParent;
  int algoParent;
  bool vector_format;
  std::vector<unsigned char> vector_rep;

  CPureBlockHeader()
    {
      SetNull();
    }

  IMPLEMENT_SERIALIZE
    (
     if (vector_format) {
       READWRITE(this->vector_rep);
     }
     else {
       READWRITE(this->nVersion);
       nVersion = this->nVersion;
       READWRITE(hashPrevBlock);
       READWRITE(hashMerkleRoot);
       if ((!isParent && GetAlgo()==ALGO_EQUIHASH) || (isParent && algoParent==ALGO_EQUIHASH)) {
	 READWRITE(hashReserved);
       }
       READWRITE(nTime);
       READWRITE(nBits);
       CBigNum nBits_bn;
       nBits_bn.SetCompact(nBits);
       if ((!isParent && GetAlgo()==ALGO_EQUIHASH) || (isParent && algoParent==ALGO_EQUIHASH)) {
	 READWRITE(nNonce256);
	 READWRITE(nSolution);
       }
       else {
	 READWRITE(nNonce);
       }
     }
     )

    void SetNull()
    {
      nVersion = CPureBlockHeader::CURRENT_VERSION;
      hashPrevBlock = 0;
      hashMerkleRoot = 0;
      hashReserved = 0;
      nTime = 0;
      nBits = 0;
      nNonce = 0;
      nNonce256.SetNull();
      nSolution.clear();
      isParent = false;
      algoParent = -1;
      vector_format = false;
      vector_rep.clear();
    }

  void SetAlgo(int algo)
  {
    switch(algo)
      {
      case ALGO_SHA256D:
	nVersion |= BLOCK_VERSION_SHA256D;
	break;
      case ALGO_SCRYPT:
	nVersion |= BLOCK_VERSION_SCRYPT;
	break;
      case ALGO_ARGON2:
	nVersion |= BLOCK_VERSION_ARGON2;
	break;
      case ALGO_X17:
	nVersion |= BLOCK_VERSION_X17;
	break;
      case ALGO_LYRA2REv2:
	nVersion |= BLOCK_VERSION_LYRA2REv2;
	break;
      case ALGO_EQUIHASH:
	nVersion |= BLOCK_VERSION_EQUIHASH;
	break;
      case ALGO_CRYPTONIGHT:
	nVersion |= BLOCK_VERSION_CRYPTONIGHT;
	break;
      case ALGO_YESCRYPT:
	nVersion |= BLOCK_VERSION_YESCRYPT;
	break;
      default:
	break;
      }
  }

  int GetAlgo () const {
    if (algoParent != -1) return algoParent;
    switch (nVersion & BLOCK_VERSION_ALGO)
      {
      case BLOCK_VERSION_SHA256D:
	return ALGO_SHA256D;
      case BLOCK_VERSION_SCRYPT:
	return ALGO_SCRYPT;
      case BLOCK_VERSION_ARGON2:
	return ALGO_ARGON2;
      case BLOCK_VERSION_X17:
	return ALGO_X17;
      case BLOCK_VERSION_LYRA2REv2:
	return ALGO_LYRA2REv2;
      case BLOCK_VERSION_EQUIHASH:
	return ALGO_EQUIHASH;
      case BLOCK_VERSION_CRYPTONIGHT:
	return ALGO_CRYPTONIGHT;
      case BLOCK_VERSION_YESCRYPT:
	return ALGO_YESCRYPT;
      }
    return ALGO_SCRYPT;
  }

  void SetChainId(int32_t id)
  {
    nVersion %= BLOCK_VERSION_CHAIN;
    nVersion |= id * BLOCK_VERSION_CHAIN;
  }

  int32_t GetChainId() const
  {
    //return nVersion & BLOCK_VERSION_CHAIN;
    if (vector_format) {
      if (vector_rep.size()<4) return 0;
      for (int i=0; i<4; i++) {
	((unsigned char *)&nVersion)[i] = vector_rep[4-i];
      }
    }
    return nVersion >> 16;
  }

  void SetUpdateSSF()
  {
    nVersion |= BLOCK_VERSION_UPDATE_SSF;
  }

  bool IsNull() const
  {
    return (nBits == 0);
  }

  uint256 GetHash() const;

  uint256 GetHashE() const;

  uint256 GetPoWHash(int algo) const
  {
    switch(algo) {
    case ALGO_SHA256D:
      return GetHash();
    case ALGO_SCRYPT:
      {
	//special for testing
	/*if (nTime > 1527138083 && nBits == 453187307) {
	  //LogPrintf("do special powhash\n");
	  uint256 thash;
	  hash_easy(BEGIN(nVersion),BEGIN(thash));
	  return thash;
	  }*/
	uint256 thash;
	hash_scrypt(BEGIN(nVersion),BEGIN(thash));
	return thash;
      }
    case ALGO_ARGON2:
      {
	uint256 thash;
	hash_argon2(BEGIN(nVersion),BEGIN(thash));
	return thash;
      }
    case ALGO_X17:
      return hash_x17(BEGIN(nVersion), END(nNonce));
    case ALGO_LYRA2REv2:
      {
	uint256 thash;
	hash_lyra2rev2(BEGIN(nVersion),BEGIN(thash));
	return thash;
      }
    case ALGO_EQUIHASH:
      {
	return GetHashE();
      }
    case ALGO_CRYPTONIGHT:
      {
	uint256 thash;
	if (vector_format) {
	  hash_cryptonight(BEGIN(vector_rep[0]),BEGIN(thash),vector_rep.size());
	}
	else {
	  hash_cryptonight(BEGIN(nVersion),BEGIN(thash),80);
	}
	return thash;
      } 
    case ALGO_YESCRYPT:
      {
	uint256 thash;
	hash_yescrypt(BEGIN(nVersion),BEGIN(thash));
	return thash;
      }
    }
    uint256 thash;
    hash_scrypt(BEGIN(nVersion),BEGIN(thash));
    return thash;
  }
  
  uint256 GetPoWHash() const
  {
    return GetPoWHash(GetAlgo());
  }

  int64_t GetBlockTime() const
  {
    return (int64_t)nTime;
  }

  inline void SetAuxpow(bool auxpow)
  {
    if (auxpow)
      nVersion |= BLOCK_VERSION_AUXPOW;
    else
      nVersion &= ~BLOCK_VERSION_AUXPOW;
  }
  
  inline bool IsAuxpow() const
  {
    return nVersion & BLOCK_VERSION_AUXPOW;
  }
  
};

#endif
