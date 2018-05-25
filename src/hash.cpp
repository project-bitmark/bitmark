#include "hash.h"
#include "scrypt.h"
#include "argon2.h"
#include "hashx17.h"
#include "Lyra2RE.h"
#include "cryptonight/crypto/hash-ops.h"
#include "yescrypt/yescrypt.h"

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
  uint32_t h = seed;
  if (len > 3) {
    const uint32_t* key_x4 = (const uint32_t*) key;
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
    key = (const uint8_t*) key_x4;
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

inline uint32_t MROTL32 ( uint32_t x, int8_t r )
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint32_t * blocks = (const uint32_t *)(&vDataToHash[0] + nblocks*4);

    for(int i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = MROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = MROTL32(h1,13); 
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail
    const uint8_t * tail = (const uint8_t*)(&vDataToHash[0] + nblocks*4);

    uint32_t k1 = 0;

    switch(vDataToHash.size() & 3)
    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = MROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

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

int HMAC_SHA512_Init(HMAC_SHA512_CTX *pctx, const void *pkey, size_t len)
{
    unsigned char key[128];
    if (len <= 128)
    {
        memcpy(key, pkey, len);
        memset(key + len, 0, 128-len);
    }
    else
    {
        SHA512_CTX ctxKey;
        SHA512_Init(&ctxKey);
        SHA512_Update(&ctxKey, pkey, len);
        SHA512_Final(key, &ctxKey);
        memset(key + 64, 0, 64);
    }

    for (int n=0; n<128; n++)
        key[n] ^= 0x5c;
    SHA512_Init(&pctx->ctxOuter);
    SHA512_Update(&pctx->ctxOuter, key, 128);

    for (int n=0; n<128; n++)
        key[n] ^= 0x5c ^ 0x36;
    SHA512_Init(&pctx->ctxInner);
    return SHA512_Update(&pctx->ctxInner, key, 128);
}

int HMAC_SHA512_Update(HMAC_SHA512_CTX *pctx, const void *pdata, size_t len)
{
    return SHA512_Update(&pctx->ctxInner, pdata, len);
}

int HMAC_SHA512_Final(unsigned char *pmd, HMAC_SHA512_CTX *pctx)
{
    unsigned char buf[64];
    SHA512_Final(buf, &pctx->ctxInner);
    SHA512_Update(&pctx->ctxOuter, buf, 64);
    return SHA512_Final(pmd, &pctx->ctxOuter);
}

void hash_scrypt(const char * input, char * output) {
  scrypt_1024_1_1_256(input,output);
}

void hash_easy (const char* input, char * output) {
  ((uint32_t*)output)[0] = murmur3_32((uint8_t*)input,20,((uint32_t*)input)[2]);
  ((uint32_t*)output)[1] = murmur3_32((uint8_t*)input+20,20,((uint32_t*)input)[3]);
  ((uint32_t*)output)[2] = murmur3_32((uint8_t*)input+40,20,((uint32_t*)input)[0]);
  ((uint32_t*)output)[3] = murmur3_32((uint8_t*)input+60,20,((uint32_t*)input)[1]);
}

void hash_argon2(const char * input, char * output) {
  argon2d_hash_raw(1,4096,1,input,80,input,80,output,32);
}

uint256 hash_x17(const char * begin, const char * end) {
  return HashX17(begin,end);
}

void hash_lyra2rev2(const char * input, char * output) {
  lyra2re2_hash(input,output);
}

void hash_equihash(const char * input, char * output) {
  //lyra2re2_hash(input,output);
}

void hash_cryptonight(const char * input, char * output, int len) {
  cn_slow_hash((const void*)input,len,(char*)output,1,0);
}

void hash_yescrypt(const char * input, char * output) {
  yescrypt_hash(input,output);
}
