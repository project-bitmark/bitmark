// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

//#include "cpuminer-config.h"
//#include "miner.h"
#include "cryptonight/c_groestl.h"
#include "cryptonight/c_blake256.h"
#include "cryptonight/c_jh.h"
#include "cryptonight/c_skein.h"
#include "cryptonight/cryptonight.h"

#if defined __unix__ && (!defined __APPLE__)
#include <sys/mman.h>
#elif defined _WIN32
#include <windows.h>
#endif

void do_blake_hash(const void* input, size_t len, char* output) {
    blake256_hash((uint8_t*)output, input, len);
}

void do_groestl_hash(const void* input, size_t len, char* output) {
    groestl(input, len * 8, (uint8_t*)output);
}

void do_jh_hash(const void* input, size_t len, char* output) {
    jh_hash(32 * 8, input, 8 * len, (uint8_t*)output);
}

void do_skein_hash(const void* input, size_t len, char* output) {
    skein_hash(8 * 32, input, 8 * len, (uint8_t*)output);
}

void xor_blocks_dst(const uint8_t *restrict a, const uint8_t *restrict b, uint8_t *restrict dst) {
    ((uint64_t*) dst)[0] = ((uint64_t*) a)[0] ^ ((uint64_t*) b)[0];
    ((uint64_t*) dst)[1] = ((uint64_t*) a)[1] ^ ((uint64_t*) b)[1];
}

void (* const extra_hashes[4])(const void *, size_t, char *) = {do_blake_hash, do_groestl_hash, do_jh_hash, do_skein_hash};

int cryptonight_hash(void* output, const void* input, size_t len) {
  printf("do cryptonight_hash len %d on\n",len);
  for (int i=0; i<len; i++) {
    printf("%02x",((unsigned char *)input)[i]);
  }
  printf("\n");
    struct cryptonight_ctx *ctx = (struct cryptonight_ctx*)malloc(sizeof(struct cryptonight_ctx));
    int rc = cryptonight_hash_ctx(output, input, len, ctx, 1);
    free(ctx);
    return rc;
}
