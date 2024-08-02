// Copyright (c) 2013-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <span.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>
#include <crypto/scrypt.h>
#include <crypto/ar2/argon2.h>
#include <crypto/x17/hashx17.h>
#include <crypto/lyra2/Lyra2RE.h>
#include <crypto/yescrypt/yescrypt.h>
#include <crypto/cryptonight/crypto/hash-ops.h>

#include <bit>
#include <string>

unsigned int MurmurHash3(unsigned int nHashSeed, Span<const unsigned char> vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint8_t* blocks = vDataToHash.data();

    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = ReadLE32(blocks + i*4);

        k1 *= c1;
        k1 = std::rotl(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = std::rotl(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail
    const uint8_t* tail = vDataToHash.data() + nblocks * 4;

    uint32_t k1 = 0;

    switch (vDataToHash.size() & 3) {
        case 3:
            k1 ^= tail[2] << 16;
            [[fallthrough]];
        case 2:
            k1 ^= tail[1] << 8;
            [[fallthrough]];
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = std::rotl(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    //----------
    // finalization
    h1 ^= vDataToHash.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64])
{
    unsigned char num[4];
    WriteBE32(num, nChild);
    CHMAC_SHA512(chainCode.begin(), chainCode.size()).Write(&header, 1).Write(data, 32).Write(num, 4).Finalize(output);
}

uint256 SHA256Uint256(const uint256& input)
{
    uint256 result;
    CSHA256().Write(input.begin(), 32).Finalize(result.begin());
    return result;
}

HashWriter TaggedHash(const std::string& tag)
{
    HashWriter writer{};
    uint256 taghash;
    CSHA256().Write((const unsigned char*)tag.data(), tag.size()).Finalize(taghash.begin());
    writer << taghash << taghash;
    return writer;
}

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed)
{
    uint32_t h = seed;
    if (len > 3) {
        const uint32_t* key_x4 = (const uint32_t*)key;
        size_t i = len >> 2;
        do {
            uint32_t k = *key_x4++;
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = (h * 5) + 0xe6546b64;
        } while (--i);
        key = (const uint8_t*)key_x4;
    }
    if (len & 3) {
        size_t i = len & 3;
        uint32_t k = 0;
        key = &key[i - 1];
        do {
            k <<= 8;
            k |= *key--;
        } while (--i);
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}


void hash_scrypt(const char* input, char* output)
{
    scrypt_1024_1_1_256(input, output);
}

void hash_easy(const char* input, char* output)
{
    for (int i = 0; i < 7; i++) {
        uint32_t hashpart = murmur3_32((uint8_t*)input + 10 * i, 10, ((uint32_t*)input)[16 - 2 * i]);
        // LogPrintf("murmur %d = %u\n",i,hashpart);
        ((uint32_t*)output)[i] = hashpart;
    }
    ((uint32_t*)output)[7] = 0;
}

void hash_argon2(const char* input, char* output)
{
    argon2d_hash_raw(1, 4096, 1, input, 80, input, 80, output, 32);
}

uint256 hash_x17(const char* begin, const char* end)
{
    return HashX17(begin, end);
}

void hash_lyra2rev2(const char* input, char* output)
{
    lyra2re2_hash(input, output);
}

void hash_equihash(const char* input, char* output)
{
    return;
    // lyra2re2_hash(input,output);
}

void hash_cryptonight(const char* input, char* output, int len)
{
    cn_slow_hash((const void*)input, len, (char*)output, 1, 0);
}

void hash_yescrypt(const char* input, char* output)
{
    yescrypt_hash(input, output);
}
