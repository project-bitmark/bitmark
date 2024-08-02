// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <consensus/amount.h>
#include <hash.h>
#include <script/script.h>
#include <serialize.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/transaction_identifier.h>
#include <primitives/block.h>
#include <logging.h>
#include <sync.h>
#include <validation.h>
#include <chainparams.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(Txid hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) :
    vin(tx.vin),
    vout(tx.vout),
    nVersion(tx.nVersion),
    nLockTime(tx.nLockTime),
    vector_format(tx.vector_format),
    vector_rep(tx.vector_rep)
{}

Txid CMutableTransaction::GetHash() const
{
    if (this->vector_format) {
        unsigned char* start = const_cast<unsigned char*>(vector_rep.data());
        unsigned char* end = const_cast<unsigned char*>(vector_rep.data() + vector_rep.size());

        if (this->keccak_hash) {
            return Txid::FromUint256(KeccakHashCBTX(start, end));
        } else {
            return Txid::FromUint256(Hash256(start, end));
        }
    }

    return Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(*this)).GetHash());
}

bool CTransaction::ComputeHasWitness() const
{
    return std::any_of(vin.begin(), vin.end(), [](const auto& input) {
        return !input.scriptWitness.IsNull();
    });
}

Txid CTransaction::ComputeHash() const
{
    if (this->vector_format) {
        if (this->keccak_hash) {
           return Txid::FromUint256(KeccakHashCBTX((unsigned char*)&vector_rep[0], (unsigned char*)&vector_rep[vector_rep.size()]));
        } else {
            return Txid::FromUint256(Hash256((unsigned char*)&vector_rep[0], (unsigned char*)&vector_rep[vector_rep.size()]));
        }
    }

    return Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(*this)).GetHash());
}

Wtxid CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return Wtxid::FromUint256(hash.ToUint256());
    }

    return Wtxid::FromUint256((HashWriter{} << TX_WITH_WITNESS(*this)).GetHash());
}

CTransaction::CTransaction(const CMutableTransaction& tx)
    : vin(tx.vin),
    vout(tx.vout),
    nVersion(tx.nVersion),
    nLockTime(tx.nLockTime),
    vector_format(tx.vector_format),
    vector_rep(tx.vector_rep),
    keccak_hash(tx.keccak_hash),
    m_has_witness{ComputeHasWitness()},
    hash{ComputeHash()},
    m_witness_hash{ComputeWitnessHash()}
{}

CTransaction::CTransaction(CMutableTransaction&& tx) :
    vin(std::move(tx.vin)),
    vout(std::move(tx.vout)),
    nVersion(tx.nVersion),
    nLockTime(tx.nLockTime),
    vector_format(std::move(tx.vector_format)),
    vector_rep(std::move(tx.vector_format)),
    keccak_hash(std::move(tx.keccak_hash)),
    m_has_witness{ComputeHasWitness()},
    hash{ComputeHash()},
    m_witness_hash{ComputeWitnessHash()}
{}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (const auto& tx_out : vout) {
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut + tx_out.nValue))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        nValueOut += tx_out.nValue;
    }
    assert(MoneyRange(nValueOut));
    return nValueOut;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(TX_WITH_WITNESS(*this));
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str += "    " + tx_in.ToString() + "\n";
    for (const auto& tx_in : vin)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    return str;
}


bool CMerkleTx::SetMerkleBranch(const CBlock& pblock)
{
    CBlock blockTmp;

    // Update the tx's hashBlock
    hashBlock = pblock.GetHash();

    // Locate the transaction
    for (nIndex = 0; nIndex < (int)pblock.vtx.size(); nIndex++) {
        if (*(pblock.vtx[nIndex]) == CTransaction(*this)) {
            break;
        }
    }

    if (nIndex == (int)pblock.vtx.size()) {
        vMerkleBranch.clear();
        nIndex = -1;
        LogPrintf("ERROR: SetMerkleBranch() : couldn't find tx in block\n");
        return false;
    }

    // Fill in merkle branch
    vMerkleBranch = pblock.GetMerkleBranch(nIndex);
    return true;
}


bool CAuxPow::check(const uint256& hashAuxBlock, int nChainId) const
{
    Consensus::Params params = Params().GetConsensus();
    // LogPrintf("check auxpow with parentBlock chainId = %d and vChainMerkleBranch size %d and nChainIndex %d\n",parentBlock.GetChainId(),vChainMerkleBranch.size(),nChainIndex);

    if (nIndex != 0) {
        LogPrintf("check auxpow err 1\n");
        return error("AuxPow is not a generate");
    }

    if (params.fStrictChainId && parentBlock.GetChainId() == nChainId) {
        LogPrintf("check auxpow err 2\n");
        return error("Aux POW parent has our chain ID");
    }

    if (vChainMerkleBranch.size() > 30) {
        LogPrintf("check auxpow err 3\n");
        return error("Aux POW chain merkle branch too long");
    }
    // LogPrintf("get nRootHash vChainMerkleBranch size %d\n",vChainMerkleBranch.size());

    // Check that the chain merkle root is in the coinbase
    const uint256 nRootHash = CBlock::CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    // LogPrintf("create vchRootHash: %s\n",nRootHash.GetHex().c_str());
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian

    uint256 transaction_hash = GetHash();
    // LogPrintf("transaction_hash = %s\n",transaction_hash.GetHex().c_str());
    // LogPrintf("hashBlock = %s\n",hashBlock.GetHex().c_str());
    // LogPrintf("auxpow transaction = %s\n",ToString().c_str());
    // LogPrintf("auxpow transaction_hash = %s\n",transaction_hash.ToString().c_str());
    if (parentBlock.vector_format) {
        int len = parentBlock.vector_rep.size();
        if (len > 1000) return error("parentBlock header too big");
        /*LogPrintf("parentBlock vector (%d) = \n",len);
        for (int i=0; i<len; i++) {
          LogPrintf("%02x",parentBlock.vector_rep[i]);
        }
        LogPrintf("\n");*/
    } else {
        // LogPrintf("parentBlock.nVersion = %u\n",parentBlock.nVersion);
        // LogPrintf("parentBlock.hashPrevBlock = %s\n",parentBlock.hashPrevBlock.ToString().c_str());
        // LogPrintf("parentBlock.hashMerkleRoot = %s\n",parentBlock.hashMerkleRoot.ToString().c_str());
        // LogPrintf("parentBlock.nTime = %lu\n",parentBlock.nTime);
        // LogPrintf("parentBlock.solution size = %lu\n",parentBlock.nSolution.size());
    }
    /*LogPrintf("merklebranch_hash = %s\n",merklebranch_hash.ToString().c_str());
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
      {
        LogPrintf("VMerkleBranch hash: %s\n",otherside.GetHex().c_str());
        }*/

    // Check that we are in the parent block merkle tree
    if (parentBlock.vector_format) {
        const uint256 merklebranch_hash = CBlock::CheckMerkleBranchKeccak(transaction_hash, vMerkleBranch, nIndex);
        std::vector<unsigned char> vchMerkleBranchHash(merklebranch_hash.begin(), merklebranch_hash.end());
        // std::reverse(vchMerkleBranchHash.begin(), vchMerkleBranchHash.end());
        /*LogPrintf("search for ");
        for (int i=0; i<32; i++) {
          LogPrintf("%02x",vchMerkleBranchHash[i]);
        }
        LogPrintf("\n");*/
        std::vector<unsigned char> vector_rep_block = parentBlock.vector_rep;
        std::vector<unsigned char>::iterator pc_block = std::search(vector_rep_block.begin(), vector_rep_block.end(), vchMerkleBranchHash.begin(), vchMerkleBranchHash.end());
        if (pc_block == vector_rep_block.end()) {
            LogPrintf("check auxpow err 4: \n");
            return error("Aux POW merkle root incorrect");
        }
    } else {
        const uint256 merklebranch_hash = CBlock::CheckMerkleBranch(transaction_hash, vMerkleBranch, nIndex);
        if (merklebranch_hash != parentBlock.hashMerkleRoot) {
            LogPrintf("check auxpow err 4: \n");
            return error("Aux POW merkle root incorrect");
        }
    }

    std::vector<unsigned char> script;
    if (vector_format) {
        script = vector_rep;
        if (script.size() > 1000)
            return error("script sig too big\n");
    } else {
        script.resize(vin[0].scriptSig.size());
        std::copy(vin[0].scriptSig.begin(), vin[0].scriptSig.end(), script.begin());
    }
    // LogPrintf("script size = %lu\n",script.size());

    // Check that the same work is not submitted twice to our chain.
    //

    std::vector<unsigned char>::iterator pcHead =
        std::search(script.begin(), script.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));

    /*LogPrintf("script:\n");
    for (unsigned int i=0;i<script.size();i++) {
      LogPrintf("%02x",script[i]);
    }
    LogPrintf("\n");*/

    std::vector<unsigned char>::iterator pc = std::search(script.begin(), script.end(), vchRootHash.begin(), vchRootHash.end());

    if (pc == script.end()) {
        return error("Aux hash not in parent coinbase");
    }

    // LogPrintf("check if multiple headers in coinbase\n");

    if (pcHead != script.end()) {
        // Enforce only one chain merkle root by checking that a single instance of the merged
        // mining header exists just before.

        if (script.end() != std::search(pcHead + 1, script.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader))) {
            return error("Multiple merged mining headers in coinbase");
            LogPrintf("check auxpow err 6\n");
        }

        if (pcHead + sizeof(pchMergedMiningHeader) != pc) {
            LogPrintf("check auxpow err 7\n");
            return error("Merged mining header is not just before chain merkle root");
        }
    } else {
        // For backward compatibility.
        // Enforce only one chain merkle root by checking that it starts early in the coinbase.
        // 8-12 bytes are enough to encode extraNonce and nBits.
        if (pc - script.begin() > 20) {
            LogPrintf("check auxpow err 8\n");
            return error("Aux POW chain merkle root must start in the first 20 bytes of the parent coinbase");
        }
    }

    // Ensure we are at a deterministic point in the merkle leaves by hashing
    // a nonce and our chain ID and comparing to the index.
    // LogPrintf("vchRootHash size = %lu\n",vchRootHash.size());
    pc += vchRootHash.size();
    if (script.end() - pc < 8) {
        LogPrintf("check auxpow err 9\n");
        return error("Aux POW missing chain merkle tree size and nonce in parent coinbase");
    }

    int nSize;
    memcpy(&nSize, &pc[0], 4);
    const unsigned merkleHeight = vChainMerkleBranch.size();
    if (nSize != (1 << merkleHeight)) {
        LogPrintf("check auxpow err 10\n");
        return error("Aux POW merkle branch size does not match parent coinbase");
    }

    int nNonce;
    memcpy(&nNonce, &pc[4], 4);

    int expectedIndex = getExpectedIndex(nNonce, nChainId, merkleHeight);
    if (nChainIndex != expectedIndex) {
        LogPrintf("check auxpow err 11: nChainIndex = %d while expectedIndex (%d,%d,%d) = %d\n", nNonce, nChainId, merkleHeight, nChainIndex, expectedIndex);
        return error("Aux POW wrong index");
    }

    return true;
}

int CAuxPow::getExpectedIndex(int nNonce, int nChainId, unsigned h)
{
    // Choose a pseudo-random slot in the chain merkle tree
    // but have it be fixed for a size/nonce/chain combination.
    //
    // This prevents the same work from being used twice for the
    // same chain while reducing the chance that two chains clash
    // for the same slot.

    unsigned rand = nNonce;
    rand = rand * 1103515245 + 12345;
    rand += nChainId;
    rand = rand * 1103515245 + 12345;

    return rand % (1 << h);
}
