// Copyright (c) 2009-2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "coins.h"

#include "util.h"
#include "sync.h"

CCriticalSection cs_main;
CChain chainActive;

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

void COutPoint::print() const
{
    LogPrintf("%s\n", ToString());
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, unsigned int nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn, unsigned int nSequenceIn)
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
        str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24));
    if (nSequence != std::numeric_limits<unsigned int>::max())
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

void CTxIn::print() const
{
    LogPrintf("%s\n", ToString());
}

CTxOut::CTxOut(int64_t nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString());
}

void CTxOut::print() const
{
    LogPrintf("%s\n", ToString());
}

uint256 CTransaction::GetHash() const
{
  return SerializeHash(*this);
}

uint256 CTransaction::GetCachedHash() const
{
  if (hash!=uint256(0)) {
    return hash;
  }
  else {
    return SerializeHash(*this);
  }
}

void CTransaction::UpdateHash() const
{
  if (this->vector_format) {
    if (this->keccak_hash) {
      *const_cast<uint256*>(&hash) = KeccakHashCBTX((unsigned char *)&vector_rep[0],(unsigned char *)&vector_rep[vector_rep.size()]);
    }
    else {
      *const_cast<uint256*>(&hash) = Hash((unsigned char *)&vector_rep[0],(unsigned char *)&vector_rep[vector_rep.size()]);
    }
  }
  else {
    *const_cast<uint256*>(&hash) = SerializeHash(*this);
  }
}

bool CTransaction::IsNewerThan(const CTransaction& old) const
{
    if (vin.size() != old.vin.size())
        return false;
    for (unsigned int i = 0; i < vin.size(); i++)
        if (vin[i].prevout != old.vin[i].prevout)
            return false;

    bool fNewer = false;
    unsigned int nLowest = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (vin[i].nSequence != old.vin[i].nSequence)
        {
            if (vin[i].nSequence <= nLowest)
            {
                fNewer = false;
                nLowest = vin[i].nSequence;
            }
            if (old.vin[i].nSequence < nLowest)
            {
                fNewer = true;
                nLowest = old.vin[i].nSequence;
            }
        }
    }
    return fNewer;
}

int64_t CTransaction::GetValueOut() const
{
    int64_t nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        nValueOut += txout.nValue;
        if (!MoneyRange(txout.nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error("CTransaction::GetValueOut() : value out of range");
    }
    return nValueOut;
}

double CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)txin.scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    if (nTxSize == 0) return 0.0;
    return dPriorityInputs / nTxSize;
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
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

void CTransaction::print() const
{
    LogPrintf("%s", ToString());
}

// Amount compression:
// * If the amount is 0, output 0
// * first, divide the amount (in base units) by the largest power of 10 possible; call the exponent e (e is max 9)
// * if e<9, the last digit of the resulting number cannot be 0; store it as d, and drop it (divide by 10)
//   * call the result n
//   * output 1 + 10*(9*n + d - 1) + e
// * if e==9, we only know the resulting number is not zero, so output 1 + 10*(n - 1) + 9
// (this is decodable, as d is in [1-9] and e is in [0-9])

uint64_t CTxOutCompressor::CompressAmount(uint64_t n)
{
    if (n == 0)
        return 0;
    int e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        int d = (n % 10);
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n*9 + d - 1)*10 + e;
    } else {
        return 1 + (n - 1)*10 + 9;
    }
}

uint64_t CTxOutCompressor::DecompressAmount(uint64_t x)
{
    // x = 0  OR  x = 1+10*(9*n + d - 1) + e  OR  x = 1+10*(n - 1) + 9
    if (x == 0)
        return 0;
    x--;
    // x = 10*(9*n + d - 1) + e
    int e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        // x = 9*n + d - 1
        int d = (x % 9) + 1;
        x /= 9;
        // x = n
        n = x*10 + d;
    } else {
        n = x+1;
    }
    while (e) {
        n *= 10;
        e--;
    }
    return n;
}

uint256 CBlock::BuildMerkleTree() const
{
    vMerkleTree.clear();
    BOOST_FOREACH(const CTransaction& tx, vtx)
        vMerkleTree.push_back(tx.GetHash());
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
}

std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
  //LogPrintf("in checkmerklebranch nIndex=%d\n",nIndex);
    if (nIndex == -1)
        return 0;
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
    {
      //LogPrintf("otherside\n");
        if (nIndex & 1)
            hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        nIndex >>= 1;
    }
    return hash;
}

uint256 CBlock::CheckMerkleBranchKeccak(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
        return 0;
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
    {
        if (nIndex & 1)
            hash = KeccakHash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = KeccakHash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        nIndex >>= 1;
    }
    return hash;
}

void CBlock::print() const
{
    LogPrintf("CBlock(hash=%s, pow=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        GetPoWHash().ToString().c_str(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        LogPrintf("  ");
        vtx[i].print();
    }
    LogPrintf("  vMerkleTree: ");
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        LogPrintf("%s ", vMerkleTree[i].ToString());
    LogPrintf("\n");
}

int GetBlockVersion (const int nVersion) {
  return nVersion & 255;
}

bool CBlockIndex::IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
  /* force the fork after a certain height */
  //if (minVersion==4 && pstart->nHeight>=nForkHeightForce-1) return true;
  
  unsigned int nFound = 0;
  for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++)
    {
      if (GetBlockVersion(pstart->nVersion) >= minVersion)
	++nFound;
      pstart = pstart->pprev;
    }
  return (nFound >= nRequired);
}

int64_t CBlockIndex::GetMedianTime() const
{
  AssertLockHeld(cs_main);
  const CBlockIndex* pindex = this;
  for (int i = 0; i < nMedianTimeSpan/2; i++)
    {
      if (!chainActive.Next(pindex))
	return GetBlockTime();
      pindex = chainActive.Next(pindex);
    }
  return pindex->GetMedianTimePast();
}

bool
CAuxPow::check(const uint256& hashAuxBlock, int nChainId, const CChainParams& params) const
{

  //LogPrintf("check auxpow with parentBlock chainId = %d and vChainMerkleBranch size %d and nChainIndex %d\n",parentBlock.GetChainId(),vChainMerkleBranch.size(),nChainIndex);
  
  if (nIndex != 0) {
    LogPrintf("check auxpow err 1\n");
    return error("AuxPow is not a generate");
  }

  if (params.StrictChainId() && parentBlock.GetChainId() == nChainId) {
    LogPrintf("check auxpow err 2\n");
    return error("Aux POW parent has our chain ID");
  }
  
  if (vChainMerkleBranch.size() > 30) {
    LogPrintf("check auxpow err 3\n");
    return error("Aux POW chain merkle branch too long");
  }
  //LogPrintf("get nRootHash vChainMerkleBranch size %d\n",vChainMerkleBranch.size());

    // Check that the chain merkle root is in the coinbase
    const uint256 nRootHash = CBlock::CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    //LogPrintf("create vchRootHash: %s\n",nRootHash.GetHex().c_str());
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian

    uint256 transaction_hash = GetHash();
    //LogPrintf("transaction_hash = %s\n",transaction_hash.GetHex().c_str());
    //LogPrintf("hashBlock = %s\n",hashBlock.GetHex().c_str());
    //LogPrintf("auxpow transaction = %s\n",ToString().c_str());
    //LogPrintf("auxpow transaction_hash = %s\n",transaction_hash.ToString().c_str());
    if (parentBlock.vector_format) {
      int len = parentBlock.vector_rep.size();
      if (len > 1000) return error("parentBlock header too big");
      /*LogPrintf("parentBlock vector (%d) = \n",len);
      for (int i=0; i<len; i++) {
	LogPrintf("%02x",parentBlock.vector_rep[i]);
      }
      LogPrintf("\n");*/
    }
    else {
      //LogPrintf("parentBlock.nVersion = %u\n",parentBlock.nVersion);
      //LogPrintf("parentBlock.hashPrevBlock = %s\n",parentBlock.hashPrevBlock.ToString().c_str());
      //LogPrintf("parentBlock.hashMerkleRoot = %s\n",parentBlock.hashMerkleRoot.ToString().c_str());
      //LogPrintf("parentBlock.nTime = %lu\n",parentBlock.nTime);
      //LogPrintf("parentBlock.solution size = %lu\n",parentBlock.nSolution.size());
    }
    /*LogPrintf("merklebranch_hash = %s\n",merklebranch_hash.ToString().c_str());
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
      {
	LogPrintf("VMerkleBranch hash: %s\n",otherside.GetHex().c_str());
	}*/    
    
    // Check that we are in the parent block merkle tree
    if (parentBlock.vector_format) {
      const uint256 merklebranch_hash = CBlock::CheckMerkleBranchKeccak(transaction_hash, vMerkleBranch, nIndex);
      std::vector<unsigned char> vchMerkleBranchHash(merklebranch_hash.begin(),merklebranch_hash.end());
      //std::reverse(vchMerkleBranchHash.begin(), vchMerkleBranchHash.end());
      /*LogPrintf("search for ");
      for (int i=0; i<32; i++) {
	LogPrintf("%02x",vchMerkleBranchHash[i]);
      }
      LogPrintf("\n");*/
      std::vector<unsigned char> vector_rep_block = parentBlock.vector_rep;
      std::vector<unsigned char>::iterator pc_block = std::search(vector_rep_block.begin(),vector_rep_block.end(), vchMerkleBranchHash.begin(), vchMerkleBranchHash.end());
      if (pc_block == vector_rep_block.end()) {
	LogPrintf("check auxpow err 4: \n");
	return error("Aux POW merkle root incorrect");
      }
    }
    else {
      const uint256 merklebranch_hash = CBlock::CheckMerkleBranch(transaction_hash, vMerkleBranch, nIndex);
      if (merklebranch_hash != parentBlock.hashMerkleRoot) {
	LogPrintf("check auxpow err 4: \n");
        return error("Aux POW merkle root incorrect");
      }
    }

    std::vector<unsigned char> script;
    if (vector_format) {
      script = vector_rep;
      if (script.size()>1000) return error("script sig too big\n");
    }
    else {
      script = vin[0].scriptSig;
    }
    //LogPrintf("script size = %lu\n",script.size());

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

    //LogPrintf("check if multiple headers in coinbase\n");
       
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
    //LogPrintf("vchRootHash size = %lu\n",vchRootHash.size());
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
      LogPrintf("check auxpow err 11: nChainIndex = %d while expectedIndex (%d,%d,%d) = %d\n",nNonce,nChainId,merkleHeight,nChainIndex,expectedIndex);
      return error("Aux POW wrong index");
    }

    return true;
}

int
CAuxPow::getExpectedIndex(int nNonce, int nChainId, unsigned h)
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

FILE* OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly)
{
    if (pos.IsNull())
        return NULL;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE* file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    /*int counter = 0;
    while (!file && counter < 1) {
        LogPrintf("Unable to open file %s\n", path.string());
	if (fReadOnly) {
	  file = fopen(path.string().c_str(), "rb+");
	}
	else {
	  file = fopen(path.string().c_str(), "wb+");
	}
        //return NULL;
	counter ++;
	}*/
    if (!file) {
      LogPrintf("unable to open file %s\n",path.string());
      return NULL;
    }
    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
	  LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return NULL;
        }
    }
    return file;
}

FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "blk", fReadOnly);
}

bool CheckAuxPowProofOfWork(const CBlockHeader& block, const CChainParams& params)
{
  int algo = block.GetAlgo();
  /*if (block.auxpow || block.IsAuxpow()) {
    LogPrintf("checking auxpowproofofwork for algo %d\n",algo);
    LogPrintf("chain id : %d\n",block.GetChainId());
    }*/

  if (block.nVersion > 3 && block.IsAuxpow() && params.StrictChainId() && block.GetChainId() != params.GetAuxpowChainId()) {
    LogPrintf("auxpow err 1\n");
    return error("%s : block does not have our chain ID"
		 " (got %d, expected %d, full nVersion %d)",
		 __func__,
		 block.GetChainId(),
		 params.GetAuxpowChainId(),
		 block.nVersion);
  }

  if (!block.auxpow) {
    if (block.IsAuxpow()) {
      LogPrintf("auxpow err 2\n");
      return error("%s : no auxpow on block with auxpow version",
		   __func__);
    }

    if (!CheckProofOfWork(block.GetPoWHash(algo), block.nBits,block.GetAlgo())) {
      LogPrintf("auxpow err 3\n");
      return error("%s : non-AUX proof of work failed", __func__);
    }

    return true;
  }

  if (!block.IsAuxpow()) {
    LogPrintf("auxpow err 4\n");
    return error("%s : auxpow on block with non-auxpow version", __func__);
  }

  if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params)) {
    LogPrintf("auxpow err 5\n");
    return error("%s : AUX POW is not valid", __func__);
  }

  if(fDebug)
    {
      CBigNum bnTarget;
      bnTarget.SetCompact(block.nBits);
      uint256 target = bnTarget.getuint256();

      LogPrintf("DEBUG: proof-of-work submitted  \n  parent-PoWhash: %s\n  target: %s  bits: %08x \n",
		block.auxpow->getParentBlockPoWHash(algo).ToString().c_str(),
		target.GetHex().c_str(),
		bnTarget.GetCompact());
    }

  if (block.GetAlgo() == ALGO_EQUIHASH && !CheckEquihashSolution(&(block.auxpow->parentBlock), Params())) {
    return error("%s : AUX equihash solution failed", __func__);
  }
  
  if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(algo), block.nBits, block.GetAlgo()))
    {
      return error("%s : AUX proof of work failed", __func__);
    }
  
  return true;
}

// Based on tests with general purpose CPUs,
//       ( Except for SHA256 which was designed for simplicity and suited for ASICs, 
//       so given a factor of 16 decrease in weight. )
//   Weighing gives more value to hashes from some algos over others,
//      because, for example a Cryptonight hash is much more computationally expensive 
//      than a SHA256d hash.
//   Weights should ultimately reflect the market value of hashes by different algorithms;
//      this will vary constantly (and more significantly long-term with hardware developement) 
//   As of June, 2018 these values are closely reflective of market values seen on
//      nicehash.com and miningrigrentals.com
unsigned int GetAlgoWeight (const int algo) {
  unsigned int weight = 8000; // scrypt, lyra2rev2 and 17 share this value.
  switch (algo)
    {
    case ALGO_SHA256D:
      weight = 1;
      break;
    case ALGO_ARGON2:
      weight = 4000000;
      break;
    case ALGO_EQUIHASH:
      weight = 8000000;
      break;
    case ALGO_CRYPTONIGHT:
      weight = 8000000;
      break;
    case ALGO_YESCRYPT:
      weight = 800000;
      break;
    }
  return weight;
}
