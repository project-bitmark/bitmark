// Copyright (c) 2009-2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITMARK_CORE_H
#define BITMARK_CORE_H

#include "script.h"
#include "serialize.h"
#include "uint256.h"
#include "scrypt.h"
#include <stdint.h>
#include "hash.h"
#include "bignum.h"
#include "chainparams.h"
#include "pureheader.h"

// SSF - Subsidy Scaling Factor
static const int nSSF = 720/NUM_ALGOS; //interval for ssf updates

class CBlockHeader;

#include "pow.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/* Use the rightmost 8 bits for standard version number, 9th bit for merge mining (todo), 10-12 th bits for POW algo, 13 th bit for update scaling factor flag */

class CTransaction;

/** No amount larger than this (in satoshi) is valid */
// todo cap this after taxation is accounted for, block reward dies after 36 changes
static const int64_t MAX_MONEY = 28000000 * COIN;
inline bool MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
static const int64_t nForkHeightForce = 452000; // not used

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    unsigned int n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { hash = 0; n = (unsigned int) -1; }
    bool IsNull() const { return (hash == 0 && n == (unsigned int) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** An inpoint - a combination of a transaction and an index n into its vin */
class CInPoint
{
public:
    const CTransaction* ptx;
    unsigned int n;

    CInPoint() { SetNull(); }
    CInPoint(const CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
    void SetNull() { ptx = NULL; n = (unsigned int) -1; }
    bool IsNull() const { return (ptx == NULL && n == (unsigned int) -1); }
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    int64_t nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(int64_t nValueIn, CScript scriptPubKeyIn);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    )

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

    uint256 GetHash() const;

    bool IsDust(int64_t nMinRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::nMinRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend,
        // so dust is a txout less than 546 satoshis 
        // with default nMinRelayTxFee.
        return ((nValue*1000)/(3*((int)GetSerializeSize(SER_DISK,0)+148)) < nMinRelayTxFee);
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    unsigned int nSequence;

    CTxIn()
    {
        nSequence = std::numeric_limits<unsigned int>::max();
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max());
    CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=std::numeric_limits<unsigned int>::max());

    IMPLEMENT_SERIALIZE
    (
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    )

    bool IsFinal() const
    {
        return (nSequence == std::numeric_limits<unsigned int>::max());
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    void print() const;
};

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
 private:
  uint256 hash;
  void UpdateHash() const;
  
public:
    static int64_t nMinTxFee;
    static int64_t nMinRelayTxFee;
    static const int CURRENT_VERSION=1;
    int nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    unsigned int nLockTime;

    bool vector_format;
    std::vector<unsigned char> vector_rep;
    bool keccak_hash;

    CTransaction()
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
       READWRITE(vin);
       READWRITE(vout);
       READWRITE(nLockTime);
     }
     if (fRead) UpdateHash();
    )

    void SetNull()
    {
        nVersion = CTransaction::CURRENT_VERSION;
        vin.clear();
        vout.clear();
        nLockTime = 0;
	*const_cast<uint256*>(&hash) = uint256(0);
	vector_format = false;
	vector_rep.clear();
	keccak_hash = false;
    }

    bool IsNull() const
    {
        return (vin.empty() && vout.empty());
    }

    uint256 GetHash() const;
    uint256 GetCachedHash() const;
    bool IsNewerThan(const CTransaction& old) const;

    // Return sum of txouts.
    int64_t GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
      return (a.nVersion  == b.nVersion &&
                a.vin       == b.vin &&
                a.vout      == b.vout &&
                a.nLockTime == b.nLockTime);
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return !(a == b);
    }


    std::string ToString() const;
    void print() const;
};

class CAuxPow;
class CBlock;

struct CDiskBlockPos
{
    int nFile;
    unsigned int nPos;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nFile));
        READWRITE(VARINT(nPos));
    )

    CDiskBlockPos() {
        SetNull();
    }

  CDiskBlockPos(int nFileIn, unsigned int nPosIn) {
    nFile = nFileIn;
    nPos = nPosIn;
  }

  friend bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

  friend bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return !(a == b);
    }

  void SetNull() { nFile = -1; nPos = 0; }
  bool IsNull() const { return (nFile == -1); }
  
};

class CBlockIndex;

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx : public CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex* &pindexRet) const;

public:
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable bool fMerkleVerified;


    CMerkleTx()
    {
        Init();
    }

    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        hashBlock = 0;
        nIndex = -1;
        fMerkleVerified = false;
    }


    IMPLEMENT_SERIALIZE
    (
	READWRITE(*(CTransaction*)this);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    )


    int SetMerkleBranch(const CBlock* pblock=NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { CBlockIndex *pindexRet; return GetDepthInMainChainINTERNAL(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree=true);
};

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pchMergedMiningHeader[] = {0xfa, 0xbe, 'm', 'm'};

/**
 * Data for the merge-mining auxpow.  This is a merkle tx (the parent block's
 * coinbase tx) that can be verified to be in the parent block, and this
 * transaction's input (the coinbase script) contains the reference
 * to the actual merge-mined block.
 */
class CAuxPow : public CMerkleTx
{
    /* Public for the unit tests.  */
public:
    /** The merkle branch connecting the aux block to our coinbase.  */
    std::vector<uint256> vChainMerkleBranch;

    /** Merkle tree index of the aux block header in the coinbase.  */
    int nChainIndex;

    /** Parent block header (on which the real PoW is done).  */
    CPureBlockHeader parentBlock;

    int algo = 0; 

public:
    /* Prevent accidental conversion.  */
    inline explicit CAuxPow(const CTransaction& txIn)
        : CMerkleTx(txIn)
    {
      parentBlock.isParent = true;
    }

    inline CAuxPow()
        : CMerkleTx()
    {
      parentBlock.isParent = true;
    }

    IMPLEMENT_SERIALIZE
      (
       READWRITE(*(CMerkleTx*)this);
       nVersion = this->nVersion;
       READWRITE(vChainMerkleBranch);
       READWRITE(nChainIndex);
       READWRITE(parentBlock);
       )
    
    /**
   * Check the auxpow, given the merge-mined block's hash and our chain ID.
   * Note that this does not verify the actual PoW on the parent block!  It
   * just confirms that all the merkle branches are valid.
   * @param hashAuxBlock Hash of the merge-mined block.
   * @param nChainId The auxpow chain ID of the block to check.
   * @param params Consensus parameters.
   * @return True if the auxpow is valid.
   */
    bool check(const uint256& hashAuxBlock, int nChainId, const CChainParams& params) const;

    /**
   * Get the parent block's hash.  This is used to verify that it
   * satisfies the PoW requirement.
   * @return The parent block hash.
   */
    inline uint256
    getParentBlockPoWHash(int algo) const
    {
        return parentBlock.GetPoWHash(algo);
    }

    /**
   * Return parent block.  This is only used for the temporary parentblock
   * auxpow version check.
   * @return The parent block.
   */
    /* FIXME: Remove after the hardfork.  */
    inline const CPureBlockHeader&
    getParentBlock() const
    {
        return parentBlock;
    }

    /**
   * Calculate the expected index in the merkle tree.  This is also used
   * for the test-suite.
   * @param nNonce The coinbase's nonce value.
   * @param nChainId The chain ID.
   * @param h The merkle block height.
   * @return The expected index for the aux hash.
   */
    static int getExpectedIndex(int nNonce, int nChainId, unsigned h);

    inline uint256 GetHash() const
    {
      return CTransaction::GetCachedHash();
    }
};

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader : public CPureBlockHeader
{
public:
    boost::shared_ptr<CAuxPow> auxpow;

    CBlockHeader()
    {
        SetNull();
    }

        unsigned int GetSerializeSize(int nType, int nVersion) const \
    {                                           \
        CSerActionGetSerializeSize ser_action;  \
        const bool fGetSize = true;             \
        const bool fWrite = false;              \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        ser_streamplaceholder s;                \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
        s.nType = nType;                        \
        s.nVersion = nVersion;                  \
	READWRITE(*(CPureBlockHeader*)this);
	nVersion = this->nVersion;
	if (this->IsAuxpow()) {
	  assert(auxpow);
	  (*auxpow).parentBlock.isParent = true;
	  int algo = CPureBlockHeader::GetAlgo();
	  (*auxpow).parentBlock.algoParent = algo;
	  if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*auxpow).vector_format = true;
	  if (algo == ALGO_CRYPTONIGHT) {
	    (*auxpow).parentBlock.vector_format = true;
	    (*auxpow).keccak_hash = true;
	  }
	  READWRITE(*auxpow);
	}
        return nSerSize;                        \
    }                                           \
    template<typename Stream>                   \
    void Serialize(Stream& s, int nType, int nVersion) const \
    {                                           \
        CSerActionSerialize ser_action;         \
        const bool fGetSize = false;            \
        const bool fWrite = true;               \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
	READWRITE(*(CPureBlockHeader*)this);
	nVersion = this->nVersion;
	if (this->IsAuxpow()) {
	  assert(auxpow);
	  (*auxpow).parentBlock.isParent = true;
	  int algo = CPureBlockHeader::GetAlgo();
	  (*auxpow).parentBlock.algoParent = algo;
          if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*auxpow).vector_format = true;
          if (algo == ALGO_CRYPTONIGHT) {
	    (*auxpow).parentBlock.vector_format = true;
	    (*auxpow).keccak_hash = true;
	  }
	  READWRITE(*auxpow);
	}
    }                                           \
    template<typename Stream>                   \
    void Unserialize(Stream& s, int nType, int nVersion)  \
    {                                           \
        CSerActionUnserialize ser_action;       \
        const bool fGetSize = false;            \
        const bool fWrite = false;              \
        const bool fRead = true;                \
        unsigned int nSerSize = 0;              \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
	READWRITE(*(CPureBlockHeader*)this);
	nVersion = this->nVersion;
	if (this->IsAuxpow()) {
	  auxpow.reset(new CAuxPow());
	  assert(auxpow);
	  (*auxpow).parentBlock.isParent = true;
	  int algo = CPureBlockHeader::GetAlgo();
	  (*auxpow).parentBlock.algoParent = algo;
          if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*auxpow).vector_format = true;
          if (algo == ALGO_CRYPTONIGHT) {
	    (*auxpow).parentBlock.vector_format = true;
	    (*auxpow).keccak_hash = true;
	  }
	  READWRITE(*auxpow);
	}
	else {
	  auxpow.reset();
	}
    }
    
    void SetNull()
    {
      CPureBlockHeader::SetNull();
      auxpow.reset();
    }
    
    void SetAuxpow(CAuxPow* apow)
    {
      if (apow)
	{
	  int algo = GetAlgo();
	  if (algo==ALGO_EQUIHASH || algo==ALGO_CRYPTONIGHT) {
	    apow->vector_format = true;
	  }
	  apow->parentBlock.isParent = true;
	  apow->parentBlock.algoParent = algo;
	  if (algo==ALGO_CRYPTONIGHT) {
	    apow->parentBlock.vector_format = true;
	    apow->keccak_hash = true;
	  }
	  auxpow.reset(apow);
	  CPureBlockHeader::SetAuxpow(true);
	} else
	{
	  auxpow.reset();
	  CPureBlockHeader::SetAuxpow(false);
	}
    }

    void SetAuxpow(bool apow)
    {
      CPureBlockHeader::SetAuxpow(apow);
    }
};

class CEquihashInput : private CPureBlockHeader
{
public:
    CEquihashInput(const CPureBlockHeader &header)
    {
        CPureBlockHeader::SetNull();
        *((CPureBlockHeader*)this) = header;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
	nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
	READWRITE(hashReserved);
        READWRITE(nTime);
        READWRITE(nBits);
    )
};

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutCompressor
{
private:
    CTxOut &txout;

public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);

    CTxOutCompressor(CTxOut &txoutIn) : txout(txoutIn) { }

    IMPLEMENT_SERIALIZE(({
        if (!fRead) {
            uint64_t nVal = CompressAmount(txout.nValue);
            READWRITE(VARINT(nVal));
        } else {
            uint64_t nVal = 0;
            READWRITE(VARINT(nVal));
            txout.nValue = DecompressAmount(nVal);
        }
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
    });)
};

/** Undo information for a CTxIn
 *
 *  Contains the prevout's CTxOut being spent, and if this was the
 *  last output of the affected transaction, its metadata as well
 *  (coinbase or not, height, transaction version)
 */
class CTxInUndo
{
public:
    CTxOut txout;         // the txout data before being spent
    bool fCoinBase;       // if the outpoint was the last unspent: whether it belonged to a coinbase
    unsigned int nHeight; // if the outpoint was the last unspent: its height
    int nVersion;         // if the outpoint was the last unspent: its version

    CTxInUndo() : txout(), fCoinBase(false), nHeight(0), nVersion(0) {}
    CTxInUndo(const CTxOut &txoutIn, bool fCoinBaseIn = false, unsigned int nHeightIn = 0, int nVersionIn = 0) : txout(txoutIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn), nVersion(nVersionIn) { }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        return ::GetSerializeSize(VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion) +
               (nHeight > 0 ? ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion) : 0) +
               ::GetSerializeSize(CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        ::Serialize(s, VARINT(nHeight*2+(fCoinBase ? 1 : 0)), nType, nVersion);
        if (nHeight > 0)
            ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Serialize(s, CTxOutCompressor(REF(txout)), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        nHeight = nCode / 2;
        fCoinBase = nCode & 1;
        if (nHeight > 0)
            ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        ::Unserialize(s, REF(CTxOutCompressor(REF(txout))), nType, nVersion);
    }
};

/** Undo information for a CTransaction */
class CTxUndo
{
public:
    // undo information for all txins
    std::vector<CTxInUndo> vprevout;

    IMPLEMENT_SERIALIZE(
        READWRITE(vprevout);
    )
};

class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransaction> vtx;

    // memory only
    mutable std::vector<uint256> vMerkleTree;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    )

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        vMerkleTree.clear();
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    uint256 BuildMerkleTree() const;

    const uint256 &GetTxHash(unsigned int nIndex) const {
        assert(vMerkleTree.size() > 0); // BuildMerkleTree must have been called first
        assert(nIndex < vtx.size());
        return vMerkleTree[nIndex];
    }

    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    static uint256 CheckMerkleBranchKeccak(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    void print() const;
};

enum BlockStatus {
    BLOCK_VALID_UNKNOWN      =    0,
    BLOCK_VALID_HEADER       =    1, // parsed, version ok, hash satisfies claimed PoW, 1 <= vtx count <= max, timestamp not in future
    BLOCK_VALID_TREE         =    2, // parent found, difficulty matches, timestamp >= median previous, checkpoint
    BLOCK_VALID_TRANSACTIONS =    3, // only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids, sigops, size, merkle root
    BLOCK_VALID_CHAIN        =    4, // outputs do not overspend inputs, no double spends, coinbase output ok, immature coinbase spends, BIP30
    BLOCK_VALID_SCRIPTS      =    5, // scripts/signatures ok
    BLOCK_VALID_MASK         =    7,

    BLOCK_HAVE_DATA          =    8, // full block available in blk*.dat
    BLOCK_HAVE_UNDO          =   16, // undo data available in rev*.dat
    BLOCK_HAVE_MASK          =   24,

    BLOCK_FAILED_VALID       =   32, // stage after last reached validness failed
    BLOCK_FAILED_CHILD       =   64, // descends from failed block
    BLOCK_FAILED_MASK        =   96
};

FILE* OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly);

FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);

bool CheckAuxPowProofOfWork(const CBlockHeader& block, const CChainParams& params);

unsigned int GetAlgoWeight (const int algo);

static const int64_t nForkHeight = 200; // We set it in past so not really used for fork condition

/** The block chain is a tree shaped structure starting with the
 * genesis block at the root, with each block potentially having multiple
 * candidates to be the next block. A blockindex may have multiple pprev pointing
 * to it, but at most one of them can be part of the currently active branch.
 */
class CBlockIndex
{
public:
    // pointer to the hash of the block, if any. memory is owned by this CBlockIndex
    const uint256* phashBlock;

    // pointer to the index of the predecessor of this block
    CBlockIndex* pprev;

    // pointer to the AuxPoW header, if this block has one
    boost::shared_ptr<CAuxPow> pauxpow;

    // height of the entry in the chain. The genesis block has height 0
    int nHeight;

    // track the amount of coins emitted since genesis block, allowing us to determine max block reward
    int64_t nMoneySupply;

    // the scaling factor for the block
    unsigned int subsidyScalingFactor;
    
    // Which # file this block is stored in (blk?????.dat)
    int nFile;

    // Byte offset within blk?????.dat where this block's data is stored
    unsigned int nDataPos;

    // Byte offset within rev?????.dat where this block's undo data is stored
    unsigned int nUndoPos;

    // (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    uint256 nChainWork;

    // Number of transactions in this block.
    // Note: in a potential headers-first mode, this number cannot be relied upon
    unsigned int nTx;

    // (memory only) Number of transactions in the chain up to and including this block
    unsigned int nChainTx; // change to 64-bit type when necessary; won't happen before 2030

    // Verification status of this block. See enum BlockStatus
    unsigned int nStatus;

    // block header
    int nVersion;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;
    uint256 nNonce256;
    std::vector<unsigned char> nSolution;
    uint256 hashReserved;

    // (memory only) Sequencial id assigned to distinguish order in which blocks are received.
    uint32_t nSequenceId;

    void SetNull()
    {
        phashBlock = NULL;
        pprev = NULL;
	pauxpow.reset();
        nHeight = 0;
        nMoneySupply = 0;
	subsidyScalingFactor = 0;
        nFile = 0;
        nDataPos = 0;
        nUndoPos = 0;
        nChainWork = 0;
        nTx = 0;
        nChainTx = 0;
        nStatus = 0;
        nSequenceId = 0;

        nVersion       = 0;
        hashMerkleRoot = 0;
        nTime          = 0;
        nBits          = 0;
        nNonce         = 0;
    }

    CBlockIndex()
      {
	SetNull();
      }

    CBlockIndex(const CBlockHeader& block)
    {
      SetNull();

      nVersion       = block.nVersion;
      hashMerkleRoot = block.hashMerkleRoot;
      nTime          = block.nTime;
      nBits          = block.nBits;
      nNonce         = block.nNonce;
      hashReserved = block.hashReserved;
      nNonce256 = block.nNonce256;
      nSolution = block.nSolution;
      
    }

    CDiskBlockPos GetBlockPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos  = nDataPos;
        }
        return ret;
    }

    CDiskBlockPos GetUndoPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos  = nUndoPos;
        }
        return ret;
    }

    bool onFork() const {
      if (this->nHeight >= nForkHeight && IsSuperMajority(4,this->pprev,75,100)) return true;
      return false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
	if (IsAuxpow() && onFork())
	  {
	    const CDiskBlockPos pos = GetBlockPos();
	    CAutoFile filein(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
	    if (filein==NULL)
	      LogPrintf("ReadBlockFromDisk: OpenBlockFile failed for %s", "");
	    try {
	      filein >> block;
	    }
	    catch (const std::exception& e) {
	      LogPrintf("%s: Deserialize or I/O error - %s at %s", __func__, e.what(), "");
	    }
	    if (!CheckAuxPowProofOfWork(block, Params()))
	      LogPrintf("ReadBlockFromDisk: Errors in block header at %s", "");
	    if (block.GetHash() != GetBlockHash())
	      LogPrintf("ReadBlockFromDisk(CBlock&, CBlockIndex*): GetHash() doesn't match index for %s at %s",
			   ToString(), "");
	    return block;
	  }
        block.nVersion       = nVersion;
        if (pprev)
            block.hashPrevBlock = pprev->GetBlockHash();
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    uint256 GetBlockHash() const
    {
        return *phashBlock;
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    CBigNum GetBlockWork() const
    {
        CBigNum bnTarget;
        bnTarget.SetCompact(nBits);
        if (bnTarget <= 0)
            return 0;
	unsigned int algo_weight = GetAlgoWeight(this->GetAlgo());
	CBigNum weight(algo_weight);
	/*if (RegTest()) {
	  int exponent = Params().WorkExponent();
	  LogPrintf("exponent = %d\n",exponent);
	  CBigNum baseWork = CBigNum(0);
	  if (exponent>=0) {
	    baseWork = (CBigNum(1)<<exponent);
	  }
	  LogPrintf("baseWork = %.8f\n",baseWork.getuint256().getdouble());
	  CBigNum varWork = (CBigNum(1)<<256) / (bnTarget/weight+1);
	  LogPrintf("varWork = %.8f\n",varWork.getuint256().getdouble());
	  return baseWork + varWork;
	  }*/
	//LogPrintf("algo is %d and weight is %lu\n",nVersion & BLOCK_VERSION_ALGO,weight.getulong());
        return (CBigNum(1)<<256) / (bnTarget/weight+1);
    }
  
    // Get Average Work of latest 50 Blocks
    // Q?<<< 
    // Is for _all_ the last sequential 50 Blocks ?
    //    or   
    //    for the n/50 blocks of a particular algo within the last sequential 50 BLocks ? 
    // Note, this introduction of variable "n" was in commit 52943a3
    //  which also modified 'main.cpp' which also modified conditions that identify a chain tip split warning (blockchain fork)
    CBigNum GetBlockWorkAv() const
    {
      CBigNum work = 0;
      const CBlockIndex * pindex = this;
      int n = 0;
      for (int i=0; i<50; i++) {
        work += pindex->GetBlockWork();
        n++;
        pindex = pindex->pprev;
        if (!pindex) break;
      }
      return work/n;
    }

    bool CheckIndex() const
    {
        return true;
    }

    enum { nMedianTimeSpan=11 };

    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
	bool wasOnFork = false;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev) {
            *(--pbegin) = pindex->GetBlockTime();
	    if (pindex->onFork()) wasOnFork = true;
	    if (wasOnFork && !pindex->onFork()) break;
	}

        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }

    int64_t GetMedianTime() const;

    /**
     * Returns true if there are nRequired or more blocks of minVersion or above
     * in the last nToCheck blocks, starting at pstart and going backwards.
     */
    static bool IsSuperMajority(int minVersion, const CBlockIndex* pstart,
                                unsigned int nRequired, unsigned int nToCheck);

    std::string ToString() const
    {
        return strprintf("CBlockIndex(pprev=%p, nHeight=%d, merkle=%s, hashBlock=%s)",
            pprev, nHeight,
            hashMerkleRoot.ToString().c_str(),
            GetBlockHash().ToString().c_str());
    }

    void print() const
    {
        LogPrintf("%s\n", ToString().c_str());
    }

    inline bool IsAuxpow() const
    {
      return nVersion & BLOCK_VERSION_AUXPOW;
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
	case BLOCK_VERSION_EQUIHASH:
	  return ALGO_EQUIHASH;
	case BLOCK_VERSION_CRYPTONIGHT:
	  return ALGO_CRYPTONIGHT;
	case BLOCK_VERSION_YESCRYPT:
	  return ALGO_YESCRYPT;
	}
      return ALGO_SCRYPT;
    }

};

/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex : public CBlockIndex
{
public:
    uint256 hashPrev;

    CDiskBlockIndex() {
      SetNull();
    }

    explicit CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex) {
      hashPrev = (pprev ? pprev->GetBlockHash() : 0);
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const \
    {                                           \
        CSerActionGetSerializeSize ser_action;  \
        const bool fGetSize = true;             \
        const bool fWrite = false;              \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        ser_streamplaceholder s;                \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
        s.nType = nType;                        \
        s.nVersion = nVersion;                  \
	if (!(nType & SER_GETHASH))
	  READWRITE(VARINT(nVersion));
        
	READWRITE(VARINT(nHeight));
	READWRITE(VARINT(nMoneySupply));
	READWRITE(VARINT(nStatus));
	READWRITE(VARINT(nTx));
	if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
	  READWRITE(VARINT(nFile));
	if (nStatus & BLOCK_HAVE_DATA)
	  READWRITE(VARINT(nDataPos));
	if (nStatus & BLOCK_HAVE_UNDO)
	  READWRITE(VARINT(nUndoPos));

	// block header
	READWRITE(this->nVersion);
	READWRITE(hashPrev);
	READWRITE(hashMerkleRoot);
	bool onFork = true; //assume true for disk IO
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(hashReserved);
	}
	READWRITE(nTime);
	READWRITE(nBits);
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(nNonce256);
	  READWRITE(nSolution);
	}
	else {
	  READWRITE(nNonce);
	}
	if (this->IsAuxpow() && onFork) {
	  assert(pauxpow);
	  (*pauxpow).parentBlock.isParent = true;
	  int algo = CBlockIndex::GetAlgo();
	  (*pauxpow).parentBlock.algoParent = algo;
          if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*pauxpow).vector_format = true;
          if (algo == ALGO_CRYPTONIGHT) {
	    (*pauxpow).parentBlock.vector_format = true;
	    (*pauxpow).keccak_hash = true;
	  }
	  READWRITE(*pauxpow);
	}
        return nSerSize;                        \
    }                                           \
    template<typename Stream>                   \
    void Serialize(Stream& s, int nType, int nVersion) const \
    {                                           \
        CSerActionSerialize ser_action;         \
        const bool fGetSize = false;            \
        const bool fWrite = true;               \
        const bool fRead = false;               \
        unsigned int nSerSize = 0;              \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
	if (!(nType & SER_GETHASH))
	  READWRITE(VARINT(nVersion));
        
	READWRITE(VARINT(nHeight));
	READWRITE(VARINT(nMoneySupply));
	READWRITE(VARINT(nStatus));
	READWRITE(VARINT(nTx));
	if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
	  READWRITE(VARINT(nFile));
	if (nStatus & BLOCK_HAVE_DATA)
	  READWRITE(VARINT(nDataPos));
	if (nStatus & BLOCK_HAVE_UNDO)
	  READWRITE(VARINT(nUndoPos));

	// block header
	READWRITE(this->nVersion);
	READWRITE(hashPrev);
	READWRITE(hashMerkleRoot);
	bool onFork = true;
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(hashReserved);
	}	
	READWRITE(nTime);
	READWRITE(nBits);
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(nNonce256);
	  READWRITE(nSolution);
	}
	else {
	  READWRITE(nNonce);
	}
	if (this->IsAuxpow() && onFork) {
	  assert(pauxpow);
	  (*pauxpow).parentBlock.isParent = true;
	  int algo = CBlockIndex::GetAlgo();
	  (*pauxpow).parentBlock.algoParent = algo;
          if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*pauxpow).vector_format = true;
          if (algo == ALGO_CRYPTONIGHT) {
	    (*pauxpow).parentBlock.vector_format = true;
	    (*pauxpow).keccak_hash = true;
	  }
	  READWRITE(*pauxpow);
	}
    }                                           \
    template<typename Stream>                   \
    void Unserialize(Stream& s, int nType, int nVersion)  \
    {                                           \
        CSerActionUnserialize ser_action;       \
        const bool fGetSize = false;            \
        const bool fWrite = false;              \
        const bool fRead = true;                \
        unsigned int nSerSize = 0;              \
        assert(fGetSize||fWrite||fRead); /* suppress warning */ \
		if (!(nType & SER_GETHASH))
	  READWRITE(VARINT(nVersion));
        
	READWRITE(VARINT(nHeight));
	READWRITE(VARINT(nMoneySupply));
	READWRITE(VARINT(nStatus));
	READWRITE(VARINT(nTx));
	if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
	  READWRITE(VARINT(nFile));
	if (nStatus & BLOCK_HAVE_DATA)
	  READWRITE(VARINT(nDataPos));
	if (nStatus & BLOCK_HAVE_UNDO)
	  READWRITE(VARINT(nUndoPos));

	// block header
	READWRITE(this->nVersion);
	READWRITE(hashPrev);
	READWRITE(hashMerkleRoot);
	bool onFork = true;
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(hashReserved);
	}
	READWRITE(nTime);
	READWRITE(nBits);
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  READWRITE(nNonce256);
	  READWRITE(nSolution);
	}
	else {
	  READWRITE(nNonce);
	}
	if (this->IsAuxpow() && onFork) {
	  pauxpow.reset(new CAuxPow());
	  assert(pauxpow);
	  (*pauxpow).parentBlock.isParent = true;
	  int algo = CBlockIndex::GetAlgo();
	  (*pauxpow).parentBlock.algoParent = algo;
          if (algo == ALGO_EQUIHASH || algo == ALGO_CRYPTONIGHT) (*pauxpow).vector_format = true;
          if (algo == ALGO_CRYPTONIGHT) {
	    (*pauxpow).parentBlock.vector_format = true;
	    (*pauxpow).keccak_hash = true;
	  }
	  READWRITE(*pauxpow);
	} else {
	  pauxpow.reset();
	}
    }
 
    void SetNull() {
      CBlockIndex::SetNull();
      pauxpow.reset();
      hashPrev = 0;
    }
      
    uint256 GetBlockHash() const
    {
        CBlockHeader block;
        block.nVersion        = nVersion;
        block.hashPrevBlock   = hashPrev;
        block.hashMerkleRoot  = hashMerkleRoot;
	bool onFork = true;
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  block.hashReserved = hashReserved;
	}
        block.nTime           = nTime;
        block.nBits           = nBits;
	if (GetAlgo()==ALGO_EQUIHASH && onFork) {
	  block.nNonce256 = nNonce256;
	  block.nSolution = nSolution;
	}
	else {
	  block.nNonce = nNonce;
	}
        return block.GetHash();
    }


    std::string ToString() const
    {
        std::string str = "CDiskBlockIndex(";
        str += CBlockIndex::ToString();
        str += strprintf("\n                hashBlock=%s, hashPrev=%s)",
            GetBlockHash().ToString().c_str(),
            hashPrev.ToString().c_str());
        return str;
    }

    void print() const
    {
        LogPrintf("%s\n", ToString().c_str());
    }
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
+ */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull()
    {
        return vHave.empty();
    }
};

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    std::vector<CBlockIndex*> vChain;

public:
    /** Returns the index entry for the genesis block of this chain, or NULL if none. */
    CBlockIndex *Genesis() const {
        return vChain.size() > 0 ? vChain[0] : NULL;
    }

    /** Returns the index entry for the tip of this chain, or NULL if none. */
    CBlockIndex *Tip() const {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : NULL;
    }

    /** Returns the index entry at a particular height in this chain, or NULL if no such height exists. */
    CBlockIndex *operator[](int nHeight) const {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return NULL;
        return vChain[nHeight];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.vChain.size() - 1] == b.vChain[b.vChain.size() - 1];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pindex) const {
        return (*this)[pindex->nHeight] == pindex;
    }

    /** Find the successor of a block in this chain, or NULL if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pindex) const {
        if (Contains(pindex))
            return (*this)[pindex->nHeight + 1];
        else
            return NULL;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->nHeight : -1. */
    int Height() const {
      return vChain.size() - 1;
    }

    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pindex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pindex = NULL) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(const CBlockLocator &locator) const;
};

/** The currently-connected chain of blocks. */
extern CChain chainActive;

/* Get base version number */
int GetBlockVersion (const int nVersion);

//#include "coins.h"

/** Global variable that points to the active CCoinsView (protected by cs_main) */
//extern CCoinsViewCache *pcoinsTip;

#endif
