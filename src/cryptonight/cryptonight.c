// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

#if defined(__arm__) || defined(_MSC_VER)
#ifndef NOASM
#define NOASM
#endif
#endif

#include "oaes_lib.h"
#include "c_keccak.h"
#include "c_groestl.h"
#include "c_blake256.h"
#include "c_jh.h"
#include "c_skein.h"
#include "int-util.h"
#include "cryptonight/hash-ops.h"
#include "compat.h"
#include "cryptonight.h"

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

static void do_blake_hash(const void* input, size_t len, char* output) {
  blake256_hash((uint8_t*)output, (const uint8_t*)input, len);
}

static void do_groestl_hash(const void* input, size_t len, char* output) {
  groestl((const BitSequence*)input, len * 8, (uint8_t*)output);
}

static void do_jh_hash(const void* input, size_t len, char* output) {
  int r = jh_hash(HASH_SIZE * 8,(const BitSequence*)input, 8 * len, (uint8_t*)output);
	assert(likely(SUCCESS == r));
}

static void do_skein_hash(const void* input, size_t len, char* output) {
  int r = skein_hash(8 * HASH_SIZE, (const SkeinBitSequence*)input, 8 * len, (uint8_t*)output);
	assert(likely(SKEIN_SUCCESS == r));
}

#if defined(NOASM) || !defined(__x86_64__)
static uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi) {
	// multiplier   = ab = a * 2^32 + b
	// multiplicand = cd = c * 2^32 + d
	// ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
	uint64_t a = hi_dword(multiplier);
	uint64_t b = lo_dword(multiplier);
	uint64_t c = hi_dword(multiplicand);
	uint64_t d = lo_dword(multiplicand);

	uint64_t ac = a * c;
	uint64_t ad = a * d;
	uint64_t bc = b * c;
	uint64_t bd = b * d;

	uint64_t adbc = ad + bc;
	uint64_t adbc_carry = adbc < ad ? 1 : 0;

	// multiplier * multiplicand = product_hi * 2^64 + product_lo
	uint64_t product_lo = bd + (adbc << 32);
	uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
	*product_hi = ac + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;
	assert(ac <= *product_hi);

	return product_lo;
}
#endif

static void cryptonight_hash_ctx(void* output, const void* input, int len, struct cryptonight_ctx* ctx)
{
  //hash_process(&ctx->state.hs, (const uint8_t*) input, len);
  /*  printf("do cryptonight_hash_ctx len %d\n",len);
  for (int i=0; i<len; i++) {
    printf("%x ",((char *)input)[i]);
  }
  printf("\n");*/
	keccak1600((const uint8_t*) input, len, (uint8_t*)(&ctx->state.hs));
	ctx->aes_ctx = (oaes_ctx*) oaes_alloc();
	size_t i, j;
	memcpy(ctx->text, ctx->state.init, INIT_SIZE_BYTE);

	oaes_key_import_data(ctx->aes_ctx, ctx->state.hs.b, AES_KEY_SIZE);
	for (i = 0; likely(i < MEMORY); i += INIT_SIZE_BYTE) {
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 0], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 1], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 2], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 3], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 4], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 5], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 6], ctx->aes_ctx->key->exp_data);
		aesb_pseudo_round_mut(&ctx->text[AES_BLOCK_SIZE * 7], ctx->aes_ctx->key->exp_data);
		memcpy(&ctx->long_state[i], ctx->text, INIT_SIZE_BYTE);
	}

	xor_blocks_dst(&ctx->state.k[0], &ctx->state.k[32], ctx->a);
	xor_blocks_dst(&ctx->state.k[16], &ctx->state.k[48], ctx->b);

	for (i = 0; likely(i < ITER / 4); ++i) {
		/* Dependency chain: address -> read value ------+
		 * written value <-+ hard function (AES or MUL) <+
		 * next address  <-+
		 */
		/* Iteration 1 */
		j = e2i(ctx->a);
		aesb_single_round(&ctx->long_state[j], ctx->c, ctx->a);
		xor_blocks_dst(ctx->c, ctx->b, &ctx->long_state[j]);
		/* Iteration 2 */
		mul_sum_xor_dst(ctx->c, ctx->a, &ctx->long_state[e2i(ctx->c)]);
		/* Iteration 3 */
		j = e2i(ctx->a);
		aesb_single_round(&ctx->long_state[j], ctx->b, ctx->a);
		xor_blocks_dst(ctx->b, ctx->c, &ctx->long_state[j]);
		/* Iteration 4 */
		mul_sum_xor_dst(ctx->b, ctx->a, &ctx->long_state[e2i(ctx->b)]);
	}

	memcpy(ctx->text, ctx->state.init, INIT_SIZE_BYTE);
	oaes_key_import_data(ctx->aes_ctx, &ctx->state.hs.b[32], AES_KEY_SIZE);
	for (i = 0; likely(i < MEMORY); i += INIT_SIZE_BYTE) {
		xor_blocks(&ctx->text[0 * AES_BLOCK_SIZE], &ctx->long_state[i + 0 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[0 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[1 * AES_BLOCK_SIZE], &ctx->long_state[i + 1 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[1 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[2 * AES_BLOCK_SIZE], &ctx->long_state[i + 2 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[2 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[3 * AES_BLOCK_SIZE], &ctx->long_state[i + 3 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[3 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[4 * AES_BLOCK_SIZE], &ctx->long_state[i + 4 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[4 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[5 * AES_BLOCK_SIZE], &ctx->long_state[i + 5 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[5 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[6 * AES_BLOCK_SIZE], &ctx->long_state[i + 6 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[6 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
		xor_blocks(&ctx->text[7 * AES_BLOCK_SIZE], &ctx->long_state[i + 7 * AES_BLOCK_SIZE]);
		aesb_pseudo_round_mut(&ctx->text[7 * AES_BLOCK_SIZE], ctx->aes_ctx->key->exp_data);
	}
	memcpy(ctx->state.init, ctx->text, INIT_SIZE_BYTE);
	//hash_permutation(&ctx->state.hs);
	keccakf((uint64_t*)(&ctx->state.hs), 24);
	/*memcpy(hash, &state, 32);*/
	extra_hashes[ctx->state.hs.b[0] & 3](&ctx->state, 200, (char *)output);
	oaes_free((OAES_CTX **) &ctx->aes_ctx);
	/*	printf("output\n");
	for (int i=0; i<32; i++) {
	  printf("%x ",((char *)output)[i]);
	}
	printf("\n");*/
}

void cryptonight_hash(void* output, const void* input, int len) {
	struct cryptonight_ctx *ctx = (struct cryptonight_ctx*)malloc(sizeof(struct cryptonight_ctx));
	cryptonight_hash_ctx(output, input, len, ctx);
	free(ctx);
}
