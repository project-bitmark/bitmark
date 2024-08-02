#ifndef BITCOIN_PUREHEADER_H
#define BITCOIN_PUREHEADER_H

#include "hash.h"
#include "uint256.h"
#include <arith_uint256.h>
#include <primitives/algo.h>
#include <iostream>

/* Use the rightmost 8 bits for standard version number, 9th bit for merge mining, 10-12 th bits for POW algo, 13 th bit for update scaling factor flag, 14-16 th bits for protocol variant */
enum {
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
    BLOCK_VERSION_VARIANT = (1 << 13),
    BLOCK_VERSION_VARIANT2 = (1 << 14),
    BLOCK_VERSION_CHAIN = (1 << 16)
};

/* Get Proof of Work Algo for the block from the block's nVersion */
inline Algo GetAlgo(int nVersion)
{
    switch (nVersion & BLOCK_VERSION_ALGO) {
    case BLOCK_VERSION_SHA256D:
        return Algo::SHA256D;
    case BLOCK_VERSION_SCRYPT:
        return Algo::SCRYPT;
    case BLOCK_VERSION_ARGON2:
        return Algo::ARGON2;
    case BLOCK_VERSION_X17:
        return Algo::X17;
    case BLOCK_VERSION_LYRA2REv2:
        return Algo::LYRA2REv2;
    case BLOCK_VERSION_EQUIHASH:
        return Algo::EQUIHASH;
    case BLOCK_VERSION_CRYPTONIGHT:
        return Algo::CRYPTONIGHT;
    case BLOCK_VERSION_YESCRYPT:
        return Algo::YESCRYPT;
    }
    return Algo::SCRYPT;
}

inline bool IsAuxpow(int nVersion)
{
    return nVersion & BLOCK_VERSION_AUXPOW;
}

class CPureBlockHeader
{ // Needed to resolve circular dependecies with CAuxPow in CBlockHeader
public:
    static const int CURRENT_VERSION = 4;
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
    Algo algoParent;
    bool vector_format;
    std::vector<unsigned char> vector_rep;

    CPureBlockHeader()
    {
        SetNull();
    }

    SERIALIZE_METHODS(CPureBlockHeader, obj)
    {
        if (obj.vector_format) {
            READWRITE(obj.vector_rep);
        } else {
            READWRITE(obj.nVersion);
            READWRITE(obj.hashPrevBlock);
            READWRITE(obj.hashMerkleRoot);

            if ((!obj.isParent && obj.GetAlgo() == Algo::EQUIHASH) || (obj.isParent && obj.algoParent == Algo::EQUIHASH)) {
                READWRITE(obj.hashReserved);
            }
            READWRITE(obj.nTime);
            READWRITE(obj.nBits);
            arith_uint256 nBits_bn;
            nBits_bn.SetCompact(obj.nBits);
            if ((!obj.isParent && obj.GetAlgo() == Algo::EQUIHASH) || (obj.isParent && obj.algoParent == Algo::EQUIHASH)) {
                READWRITE(obj.nNonce256);
                READWRITE(obj.nSolution);
            } else {
                READWRITE(obj.nNonce);
            }
        }
    }

    void SetNull()
    {
        nVersion = CPureBlockHeader::CURRENT_VERSION;
        hashPrevBlock = uint256::ZERO;
        hashMerkleRoot = uint256::ZERO;
        hashReserved = uint256::ZERO;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        nNonce256.SetNull();
        nSolution.clear();
        isParent = false;
        algoParent = Algo::UNKNOWN;
        vector_format = false;
        vector_rep.clear();
    }

    void SetAlgo(Algo algo)
    {
        switch (algo) {
        case Algo::SHA256D:
            nVersion |= BLOCK_VERSION_SHA256D;
            break;
        case Algo::SCRYPT:
            nVersion |= BLOCK_VERSION_SCRYPT;
            break;
        case Algo::ARGON2:
            nVersion |= BLOCK_VERSION_ARGON2;
            break;
        case Algo::X17:
            nVersion |= BLOCK_VERSION_X17;
            break;
        case Algo::LYRA2REv2:
            nVersion |= BLOCK_VERSION_LYRA2REv2;
            break;
        case Algo::EQUIHASH:
            nVersion |= BLOCK_VERSION_EQUIHASH;
            break;
        case Algo::CRYPTONIGHT:
            nVersion |= BLOCK_VERSION_CRYPTONIGHT;
            break;
        case Algo::YESCRYPT:
            nVersion |= BLOCK_VERSION_YESCRYPT;
            break;
        default:
            break;
        }
    }

    Algo GetAlgo() const
    {
        if (algoParent != Algo::UNKNOWN)
            return algoParent;

        return ::GetAlgo(nVersion);
    }

    void SetChainId(int32_t id)
    {
        nVersion %= BLOCK_VERSION_CHAIN;
        nVersion |= id * BLOCK_VERSION_CHAIN;
    }

    int32_t GetChainId() const
    {
        // return nVersion & BLOCK_VERSION_CHAIN;
        if (vector_format) {
            if (vector_rep.size() < 4) return 0;
            for (int i = 0; i < 4; i++) {
                ((unsigned char*)&nVersion)[i] = vector_rep[4 - i];
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

    uint256 GetPoWHash(Algo algo) const;

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

    inline void SetVariant(bool variant)
    {
        if (variant)
            nVersion |= BLOCK_VERSION_VARIANT;
        else
            nVersion &= ~BLOCK_VERSION_VARIANT;
    }

    inline void SetVariant2(bool variant)
    {
        if (variant)
            nVersion |= BLOCK_VERSION_VARIANT2;
        else
            nVersion &= ~BLOCK_VERSION_VARIANT2;
    }

    inline bool IsVariant() const
    {
        return nVersion & BLOCK_VERSION_VARIANT;
    }

    inline bool IsVariant2() const
    {
        return nVersion & BLOCK_VERSION_VARIANT2;
    }
};

#endif
