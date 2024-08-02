#include "pureheader.h"
#include "hash.h"
#include <util/strencodings.h>

uint256 CPureBlockHeader::GetHash() const
{
    if (GetAlgo() == Algo::EQUIHASH) {
        return GetHashE();
    } else if (GetAlgo() == Algo::CRYPTONIGHT && this->vector_format) {
        return Hash256(BEGIN(vector_rep[0]), END(vector_rep[vector_rep.size() - 1]));
    }
    return Hash256(BEGIN(nVersion), END(nNonce));
}

uint256 CPureBlockHeader::GetHashE() const
{
    unsigned char input[1487];
    memcpy(input, BEGIN(nVersion), 4);
    memcpy(input + 4, BEGIN(hashPrevBlock), 32);
    memcpy(input + 36, BEGIN(hashMerkleRoot), 32);
    memcpy(input + 68, BEGIN(hashReserved), 32);
    memcpy(input + 100, BEGIN(nTime), 4);
    memcpy(input + 104, BEGIN(nBits), 4);
    memcpy(input + 108, BEGIN(nNonce256), 32);
    if (nSolution.size() == 1344) {
        input[140] = 0xfd;
        input[141] = 0x40;
        input[142] = 0x05;
        memcpy(input + 143, BEGIN(nSolution[0]), 1344);
    } else {
        // LogPrintf("nSolution size %lu/n",nSolution.size());
        input[140] = 0x00;
        return Hash256((char*)input, (char*)input + 141);
    }
    return Hash256((char*)input, (char*)input + 1487);
}

uint256 CPureBlockHeader::GetPoWHash(Algo algo) const
{
    switch (algo) {
    case Algo::SHA256D:
        return Hash256(BEGIN(nVersion), END(nNonce));
    case Algo::SCRYPT: {
        // special for testing
        /*if (nTime > 1527138083 && nBits == 453187307) {
          //LogPrintf("do special powhash\n");
          uint256 thash;
          hash_easy(BEGIN(nVersion),BEGIN(thash));
          return thash;
          }*/
        uint256 thash;
        hash_scrypt(BEGIN(nVersion), BEGIN(thash));
        return thash;
    }
    case Algo::ARGON2: {
        uint256 thash;
        hash_argon2(BEGIN(nVersion), BEGIN(thash));
        return thash;
    }
    case Algo::X17:
        return hash_x17(BEGIN(nVersion), END(nNonce));
    case Algo::LYRA2REv2: {
        uint256 thash;
        hash_lyra2rev2(BEGIN(nVersion), BEGIN(thash));
        return thash;
    }
    case Algo::EQUIHASH: {
        return GetHashE();
    }
    case Algo::CRYPTONIGHT: {
        uint256 thash;
        if (vector_format) {
            hash_cryptonight(BEGIN(vector_rep[0]), BEGIN(thash), vector_rep.size());
        } else {
            hash_cryptonight(BEGIN(nVersion), BEGIN(thash), 80);
        }
        return thash;
    }
    case Algo::YESCRYPT: {
        uint256 thash;
        hash_yescrypt(BEGIN(nVersion), BEGIN(thash));
        return thash;
    }
    }
    uint256 thash;
    hash_scrypt(BEGIN(nVersion), BEGIN(thash));
    return thash;
}
