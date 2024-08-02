// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <primitives/pureheader.h>
#include <primitives/algo.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>


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
    mutable std::shared_ptr<CAuxPow> auxpow;

    CBlockHeader()
    {
        SetNull();
    }

    SERIALIZE_METHODS(CBlockHeader, obj)
    {
        READWRITE(AsBase<CPureBlockHeader>(obj));

        if (obj.IsAuxpow()) {
            if (ser_action.ForRead()) {
                obj.auxpow.reset(new CAuxPow());
            }
            assert(obj.auxpow);
            obj.auxpow->parentBlock.isParent = true;
            Algo algo = AsBase<CPureBlockHeader>(obj).GetAlgo();
            obj.auxpow->parentBlock.algoParent = algo;
            if (algo == Algo::EQUIHASH || algo == Algo::CRYPTONIGHT)
                obj.auxpow->vector_format = true;

            if (algo == Algo::CRYPTONIGHT) {
                obj.auxpow->parentBlock.vector_format = true;
                obj.auxpow->keccak_hash = true;
            }

            READWRITE(TX_NO_WITNESS(*(obj.auxpow)));
        } else {
            if (ser_action.ForRead()) {
                obj.auxpow.reset();
            }
        }
    }

    void SetNull()
    {
        CPureBlockHeader::SetNull();
        auxpow.reset();
    }

    void SetAuxpow(CAuxPow* apow)
    {
        if (apow) {
            Algo algo = GetAlgo();
            if (algo == Algo::EQUIHASH || algo == Algo::CRYPTONIGHT) {
                apow->vector_format = true;
            }
            apow->parentBlock.isParent = true;
            apow->parentBlock.algoParent = algo;
            if (algo == Algo::CRYPTONIGHT) {
                apow->parentBlock.vector_format = true;
                apow->keccak_hash = true;
            }
            auxpow.reset(apow);
            CPureBlockHeader::SetAuxpow(true);
        } else {
            auxpow.reset();
            CPureBlockHeader::SetAuxpow(false);
        }
    }

    void SetAuxpow(bool apow)
    {
        CPureBlockHeader::SetAuxpow(apow);
    }

    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    std::string ToString() const;
};

class CEquihashInput : private CPureBlockHeader
{
public:
    CEquihashInput(const CPureBlockHeader& header)
    {
        CPureBlockHeader::SetNull();
        *((CPureBlockHeader*)this) = header;
    }

    SERIALIZE_METHODS(CEquihashInput, obj)
    {
        READWRITE(obj.nVersion);
        READWRITE(obj.hashPrevBlock);
        READWRITE(obj.hashMerkleRoot);
        READWRITE(obj.hashReserved);
        READWRITE(obj.nTime);
        READWRITE(obj.nBits);
    }
};

class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // Memory-only flags for caching expensive checks
    mutable bool fChecked;                            // CheckBlock()
    mutable bool m_checked_witness_commitment{false}; // CheckWitnessCommitment()
    mutable bool m_checked_merkle_root{false};        // CheckMerkleRoot()

    mutable std::vector<uint256> vMerkleTree;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITE(AsBase<CBlockHeader>(obj), obj.vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
        m_checked_witness_commitment = false;
        m_checked_merkle_root = false;
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
    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    static uint256 CheckMerkleBranchKeccak(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);


    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    /** Historically CBlockLocator's version field has been written to network
     * streams as the negotiated protocol version and to disk streams as the
     * client version, but the value has never been used.
     *
     * Hard-code to the highest protocol version ever written to a network stream.
     * SerParams can be used if the field requires any meaning in the future,
     **/
    static constexpr int DUMMY_VERSION = 70016;

    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(std::vector<uint256>&& have) : vHave(std::move(have)) {}

    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = DUMMY_VERSION;
        READWRITE(nVersion);
        READWRITE(obj.vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
