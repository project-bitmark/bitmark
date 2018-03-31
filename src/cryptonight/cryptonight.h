// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

#if defined(__arm__) || defined(_MSC_VER)
#ifndef NOASM
#define NOASM
#endif
#endif

#include "cryptonight/oaes_lib.h"
#include "cryptonight/int-util.h"
#include "cryptonight/hash-ops.h"
#include "cryptonight/compat.h"

#if USE_INT128

#if __GNUC__ == 4 && __GNUC_MINOR__ >= 4 && __GNUC_MINOR__ < 6
typedef unsigned int uint128_t __attribute__ ((__mode__ (TI)));
#elif defined (_MSC_VER)
/* only for mingw64 on windows */
#undef  USE_INT128
#define USE_INT128 (0)
#else
typedef __uint128_t uint128_t;
#endif

#endif

#define LITE 0
#if LITE /* cryptonight-light */
#define MEMORY (1 << 20)
#define ITER   (1 << 19)
#else
#define MEMORY (1 << 21) /* 2 MiB */
#define ITER   (1 << 20)
#endif

#define AES_BLOCK_SIZE  16
#define AES_KEY_SIZE    32 /*16*/
#define INIT_SIZE_BLK   8
#define INIT_SIZE_BYTE (INIT_SIZE_BLK * AES_BLOCK_SIZE)

#pragma pack(push, 1)
union hash_state {
  uint8_t b[200];
  uint64_t w[25];
};
#pragma pack(pop)

#pragma pack(push, 1)
union cn_slow_hash_state {
	union hash_state hs;
	struct {
		uint8_t k[64];
		uint8_t init[INIT_SIZE_BYTE];
	};
};
#pragma pack(pop)

static void do_blake_hash(const void* input, size_t len, char* output);

static void do_groestl_hash(const void* input, size_t len, char* output);

static void do_jh_hash(const void* input, size_t len, char* output);

static void do_skein_hash(const void* input, size_t len, char* output);

extern int aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
extern int aesb_pseudo_round_mut(uint8_t *val, uint8_t *expandedKey);
#if !defined(_MSC_VER) && !defined(NOASM)
extern int fast_aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
extern int fast_aesb_pseudo_round_mut(uint8_t *val, uint8_t *expandedKey);
#else
#define fast_aesb_single_round     aesb_single_round
#define fast_aesb_pseudo_round_mut aesb_pseudo_round_mut
#endif

#if defined(NOASM) || !defined(__x86_64__)
static uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi);
#else
extern uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi);
#endif

static void (* const extra_hashes[4])(const void *, size_t, char *) = {
  do_blake_hash, do_groestl_hash, do_jh_hash, do_skein_hash
};

static inline size_t e2i(const uint8_t* a) {
#if !LITE
	return ((uint32_t *)a)[0] & 0x1FFFF0;
#else
	return ((uint32_t *)a)[0] & 0xFFFF0;
#endif
}

static inline void mul_sum_xor_dst(const uint8_t* a, uint8_t* c, uint8_t* dst) {
	uint64_t hi, lo = mul128(((uint64_t*) a)[0], ((uint64_t*) dst)[0], &hi) + ((uint64_t*) c)[1];
	hi += ((uint64_t*) c)[0];

	((uint64_t*) c)[0] = ((uint64_t*) dst)[0] ^ hi;
	((uint64_t*) c)[1] = ((uint64_t*) dst)[1] ^ lo;
	((uint64_t*) dst)[0] = hi;
	((uint64_t*) dst)[1] = lo;
}

static inline void xor_blocks(uint8_t* a, const uint8_t* b) {
#if USE_INT128
	*((uint128_t*) a) ^= *((uint128_t*) b);
#else
	((uint64_t*) a)[0] ^= ((uint64_t*) b)[0];
	((uint64_t*) a)[1] ^= ((uint64_t*) b)[1];
#endif
}

static inline void xor_blocks_dst(const uint8_t* a, const uint8_t* b, uint8_t* dst) {
#if USE_INT128
	*((uint128_t*) dst) = *((uint128_t*) a) ^ *((uint128_t*) b);
#else
	((uint64_t*) dst)[0] = ((uint64_t*) a)[0] ^ ((uint64_t*) b)[0];
	((uint64_t*) dst)[1] = ((uint64_t*) a)[1] ^ ((uint64_t*) b)[1];
#endif
}

struct cryptonight_ctx {
	uint8_t _ALIGN(16) long_state[MEMORY];
	union cn_slow_hash_state state;
	uint8_t _ALIGN(16) text[INIT_SIZE_BYTE];
	uint8_t _ALIGN(16) a[AES_BLOCK_SIZE];
	uint8_t _ALIGN(16) b[AES_BLOCK_SIZE];
	uint8_t _ALIGN(16) c[AES_BLOCK_SIZE];
	oaes_ctx* aes_ctx;
};

static void cryptonight_hash_ctx(void* output, const void* input, int len, struct cryptonight_ctx* ctx);

void cryptonight_hash(void* output, const void* input, int len);
