#ifndef BITCOIN_PUREHEADER_H
#define BITCOIN_PUREHEADER_H

#include "serialize.h"
#include "uint256.h"
#include "util.h"
#include "scrypt.h"
#include "hash.h"

const int NUM_ALGOS = 5;

enum {
  ALGO_SHA256D = 1,
  ALGO_SCRYPT = 2,
  ALGO_ARGON2 = 3,
  ALGO_X17 = 4,
    ALGO_LYRA2REv2 = 5
};

/* Use the rightmost 8 bits for standard version number, 9th bit for merge mining (todo), 10-12 th bits for POW algo, 13 th bit for update scaling factor flag */
enum
  {
    BLOCK_VERSION_AUXPOW = (1 << 8),
    BLOCK_VERSION_ALGO = (7 << 9),
    BLOCK_VERSION_SHA256D = (1 << 9),
    BLOCK_VERSION_SCRYPT = (2 << 9),
    BLOCK_VERSION_ARGON2 = (3 << 9),
    BLOCK_VERSION_X17 = (4 << 9),
    BLOCK_VERSION_LYRA2REv2 = (5 << 9),
    BLOCK_VERSION_UPDATE_SSF = (1 << 12),
    BLOCK_VERSION_CHAIN = (1 << 16)
  };


class CPureBlockHeader {
 public:
  static const int CURRENT_VERSION=2;
  int nVersion;
  uint256 hashPrevBlock;
  uint256 hashMerkleRoot;
  unsigned int nTime;
  unsigned int nBits;
  unsigned int nNonce;

  CPureBlockHeader()
    {
      SetNull();
    }

  IMPLEMENT_SERIALIZE
    (
     READWRITE(this->nVersion);
     nVersion = this->nVersion;
     READWRITE(hashPrevBlock);
     READWRITE(hashMerkleRoot);
     READWRITE(nTime);
     READWRITE(nBits);
     READWRITE(nNonce);
     )

    void SetNull()
    {
      nVersion = CPureBlockHeader::CURRENT_VERSION;
      hashPrevBlock = 0;
      hashMerkleRoot = 0;
      nTime = 0;
      nBits = 0;
      nNonce = 0;
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
      default:
	break;
      }
  }

  int GetAlgo () const {
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
      }
    return ALGO_SCRYPT;
  }

  void SetChainId(int id)
  {
    nVersion %= BLOCK_VERSION_CHAIN;
    nVersion |= id * BLOCK_VERSION_CHAIN;
  }

  int GetChainId() const
  {
    return nVersion & BLOCK_VERSION_CHAIN;
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

  uint256 GetPoWHash(int algo) const
  {
    switch(algo) {
    case ALGO_SHA256D:
      return GetHash();
    case ALGO_SCRYPT:
      {
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
