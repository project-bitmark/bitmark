// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HASH_H
#define BITCOIN_HASH_H

#include <attributes.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <crypto/cryptonight/common/c_keccak.h>
#include <prevector.h>
#include <serialize.h>
#include <span.h>
#include <uint256.h>
#include <openssl/sha.h>

#include <string>
#include <vector>

typedef uint256 ChainCode;

/** A hasher class for Bitcoin's 256-bit hash (double SHA-256). */
class CHash256 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash256& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};

/** A hasher class for Bitcoin's 160-bit hash (SHA-256 + RIPEMD-160). */
class CHash160 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CRIPEMD160::OUTPUT_SIZE;

    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        CRIPEMD160().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash160& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash160& Reset() {
        sha.Reset();
        return *this;
    }
};

/** Compute the 256-bit hash of an object. */
template<typename T>
inline uint256 Hash(const T& in1)
{
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template<typename T1, typename T2>
inline uint256 Hash(const T1& in1, const T2& in2) {
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Write(MakeUCharSpan(in2)).Finalize(result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template <typename T1>
inline uint256 Hash256(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), (unsigned char*)&hash1);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template <typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

/** Compute the 160-bit hash an object. */
template<typename T1>
inline uint160 Hash160(const T1& in1)
{
    uint160 result;
    CHash160().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

template <typename T1>
inline uint256 KeccakHash(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    unsigned char md[200];
    int ret = keccak((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), md, 200);
    uint256 hash;
    memcpy(&hash, md, 32);
    return hash;
}

template <typename T1>
inline uint256 KeccakHashCBTX(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    unsigned char md[200];
    unsigned char* input = (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]);
    int input_size = (pend - pbegin) * sizeof(pbegin[0]);
    /*LogPrintf("do keccakhashcbtx (%d) on\n",input_size);
    for (int i=0; i<input_size; i++) {
      LogPrintf("%02x",input[i]);
    }
    LogPrintf("\n");*/
    int ret = keccak((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), md, 200);
    const char* hash2 = "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a";
    const char* hash3 = "0000000000000000000000000000000000000000000000000000000000000000";
    for (int i = 0; i < 32; i++) {
        ret = sscanf(hash2 + 2 * i, "%2hhx", md + 32 + i);
        ret = sscanf(hash3 + 2 * i, "%2hhx", md + 64 + i);
    }
    /*LogPrintf("do 2nd keccakhashcbtx on\n");
    for (int i=0; i<96; i++) {
      LogPrintf("%02x",md[i]);
    }
    LogPrintf("\n");*/
    ret = keccak(md, 96, md, 200);
    /*LogPrintf("output=\n");
    for (int i=0; i<32; i++) {
      LogPrintf("%02x",md[i]);
    }
    LogPrintf("\n");*/
    uint256 hash;
    memcpy(&hash, md, 32);
    return hash;
}

template <typename T1, typename T2>
inline uint256 KeccakHash(const T1 p1begin, const T1 p1end, const T2 p2begin, const T2 p2end)
{
    static unsigned char pblank[1];
    size_t input_size = (p1end - p1begin) * sizeof(p1begin[0]) + (p2end - p2begin) * sizeof(p2begin[0]);
    unsigned char* input = (unsigned char*)malloc(input_size);
    memcpy(input, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    memcpy(input + (p1end - p1begin) * sizeof(p1begin[0]), (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    unsigned char md[200];
    int ret = keccak(input, input_size, md, 200);
    uint256 hash;
    memcpy(&hash, md, 32);
    return hash;
}


/** A writer stream (for serialization) that computes a 256-bit hash. */
class HashWriter
{
private:
    CSHA256 ctx;

public:
    void write(Span<const std::byte> src)
    {
        ctx.Write(UCharCast(src.data()), src.size());
    }

    /** Compute the double-SHA256 hash of all data written to this object.
     *
     * Invalidates this object.
     */
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize(result.begin());
        ctx.Reset().Write(result.begin(), CSHA256::OUTPUT_SIZE).Finalize(result.begin());
        return result;
    }

    /** Compute the SHA256 hash of all data written to this object.
     *
     * Invalidates this object.
     */
    uint256 GetSHA256() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }

    /**
     * Returns the first 64 bits from the resulting hash.
     */
    inline uint64_t GetCheapHash() {
        uint256 result = GetHash();
        return ReadLE64(result.begin());
    }

    template <typename T>
    HashWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Reads data from an underlying stream, while hashing the read data. */
template <typename Source>
class HashVerifier : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashVerifier(Source& source LIFETIMEBOUND) : m_source{source} {}

    void read(Span<std::byte> dst)
    {
        m_source.read(dst);
        this->write(dst);
    }

    void ignore(size_t num_bytes)
    {
        std::byte data[1024];
        while (num_bytes > 0) {
            size_t now = std::min<size_t>(num_bytes, 1024);
            read({data, now});
            num_bytes -= now;
        }
    }

    template <typename T>
    HashVerifier<Source>& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }
};

/** Writes data to an underlying source stream, while hashing the written data. */
template <typename Source>
class HashedSourceWriter : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashedSourceWriter(Source& source LIFETIMEBOUND) : HashWriter{}, m_source{source} {}

    void write(Span<const std::byte> src)
    {
        m_source.write(src);
        HashWriter::write(src);
    }

    template <typename T>
    HashedSourceWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Single-SHA256 a 32-byte input (represented as uint256). */
[[nodiscard]] uint256 SHA256Uint256(const uint256& input);

unsigned int MurmurHash3(unsigned int nHashSeed, Span<const unsigned char> vDataToHash);

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]);

/** Return a HashWriter primed for tagged hashes (as specified in BIP 340).
 *
 * The returned object will have SHA256(tag) written to it twice (= 64 bytes).
 * A tagged hash can be computed by feeding the message into this object, and
 * then calling HashWriter::GetSHA256().
 */
HashWriter TaggedHash(const std::string& tag);

/** Compute the 160-bit RIPEMD-160 hash of an array. */
inline uint160 RIPEMD160(Span<const unsigned char> data)
{
    uint160 result;
    CRIPEMD160().Write(data.data(), data.size()).Finalize(result.begin());
    return result;
}

void hash_scrypt(const char* input, char* output);
void hash_argon2(const char* input, char* output);
uint256 hash_x17(const char* begin, const char* end);
void hash_lyra2rev2(const char* input, char* output);
void hash_equihash(const char* input, char* output);
void hash_cryptonight(const char* input, char* output, int len);
void hash_yescrypt(const char* input, char* output);
void hash_easy(const char* input, char* output); // special hash for testing

#endif // BITCOIN_HASH_H
