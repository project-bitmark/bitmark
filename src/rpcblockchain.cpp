// Copyright (c) 2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014-2018 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "main.h"
#include "sync.h"
#include "checkpoints.h"

#include <stdint.h>

#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out, bool fIncludeHex);

double GetDifficulty(const CBlockIndex* blockindex, int algo, bool weighted, bool next)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (chainActive.Tip() == NULL)
            return 1.0;
        else
            blockindex = chainActive.Tip();
    }
    if (algo<0) {
      algo = GetAlgo(blockindex->nVersion);
    }
    bool blockOnFork = false;
    if (onFork(blockindex)) blockOnFork = true;
    if (blockOnFork) {
      int algo_tip = GetAlgo(blockindex->nVersion);
      if (algo_tip != algo) {
	blockindex = get_pprev_algo(blockindex,algo);
      }
    }
    unsigned int nBits = 0;
    unsigned int algoWeight = 1;
    // Q? <<<  Please comment on working of this code
    // 3 Cases 
    if (weighted) algoWeight = GetAlgoWeight(algo);
    if (next) {
      nBits = GetNextWorkRequired(chainActive.Tip(),algo);
    }
    else if (blockindex && blockindex->nHeight>0) {
      nBits = blockindex->nBits;
    }
    else {
      nBits = (Params().ProofOfWorkLimit()*algoWeight).GetCompact();
    }
    
    int nShift = (nBits >> 24) & 0xff;
    double dDiff =
        (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    if (blockOnFork) return dDiff*algoWeight; //weighted difficulty
    return dDiff;
}

double GetPeakHashrate (const CBlockIndex* blockindex, int algo) {
  if (blockindex == NULL)
    {
      if (chainActive.Tip() == NULL)
	return 0.;
      else
	blockindex = chainActive.Tip();
    }
  
  int algo_tip = GetAlgo(blockindex->nVersion);
  if (algo_tip != algo) {
    blockindex = get_pprev_algo(blockindex,algo);
  }
  if (!blockindex) return 0.;
  do {
    if (update_ssf(blockindex->nVersion)) {
      double hashes_peak = 0.;
      const CBlockIndex * pprev_algo = get_pprev_algo(blockindex,-1);
      for (int i=0; i<365; i++) {
	if (!pprev_algo) break;
	int time_f = pprev_algo->GetMedianTimePast();
	CBigNum hashes_bn = pprev_algo->GetBlockWork();
	int time_i = 0;
	
	for (int j=0; j<nSSF-1; j++) {
	 
	  pprev_algo = get_pprev_algo(pprev_algo,-1);

	  if (pprev_algo) {
	    time_i = pprev_algo->GetMedianTimePast();
	  }
	  else {
	    hashes_bn = CBigNum(0);
	    break;
	  }
	  //LogPrintf("j=%d add block work of block %lu\n",j,pprev_algo->nHeight);
	  hashes_bn += pprev_algo->GetBlockWork();	  
	}
	CBlockIndex * pprev_algo_time = get_pprev_algo(pprev_algo,-1);
	if (pprev_algo_time) {
	  time_i = pprev_algo_time->GetMedianTimePast();
	}
	else {
	  const CBlockIndex * blockindex_time = pprev_algo;
	  while (blockindex_time && onFork(blockindex_time)) {
	    blockindex_time = blockindex_time->pprev;
	  }
	  if (blockindex_time) {
	    time_i = blockindex_time->GetBlockTime();
	  }
	}
	pprev_algo = pprev_algo_time;
	
	if (time_f>time_i) {
	  time_f -= time_i;
	}
	else {
	  return std::numeric_limits<double>::max();
	}
	//LogPrintf("hashes = %f, time = %f\n",(double)hashes_bn.getulong(),(double)time_f);
	double hashes = (((hashes_bn/time_f)/1000000)/1000).getuint256().getdouble();
	//LogPrintf("hashes per sec = %f\n",hashes);
	if (hashes>hashes_peak) hashes_peak = hashes;
      }
      return hashes_peak;
      break;
    }
    blockindex = get_pprev_algo(blockindex,-1);
  } while (blockindex);
  return 0.;
}

double GetCurrentHashrate (const CBlockIndex* blockindex, int algo) { //as used for the scaling factor calc
  if (blockindex == NULL)
    {
      if (chainActive.Tip() == NULL)
	return 0.;
      else
	blockindex = chainActive.Tip();
    }
  int algo_tip = GetAlgo(blockindex->nVersion);
  if (algo_tip != algo) {
    blockindex = get_pprev_algo(blockindex,algo);
  }
  if (!blockindex) {
    return 0.;
  }
  do {
    if (update_ssf(blockindex->nVersion)) {
      const CBlockIndex * pcur_algo = get_pprev_algo(blockindex,-1);
      if (!pcur_algo) return 0.;
      int time_f = pcur_algo->GetMedianTimePast();
      CBigNum hashes_bn = pcur_algo->GetBlockWork();
      int time_i = 0;
      const CBlockIndex * pprev_algo = pcur_algo;
      for (int j=0; j<nSSF-1; j++) {
	pprev_algo = get_pprev_algo(pprev_algo,-1);
	if (pprev_algo) {
	  time_i = pprev_algo->GetMedianTimePast();
	}
	else {
	  return 0.;
	}
	hashes_bn += pprev_algo->GetBlockWork();
      }
      CBlockIndex * pprev_algo_time = get_pprev_algo(pprev_algo,-1);
      if (pprev_algo_time) {
	time_i = pprev_algo_time->GetMedianTimePast();
      }
      else {
	const CBlockIndex * blockindex_time = pprev_algo;
	while (blockindex_time && onFork(blockindex_time)) {
	  blockindex_time = blockindex_time->pprev;
	}
	if (blockindex_time) time_i = blockindex_time->GetBlockTime();
      }

      if (time_f>time_i) {
	time_f -= time_i;
      }
      else {
	return std::numeric_limits<double>::max();
      }
      //LogPrintf("return %lu / %f\n",(double)hashes_bn.getulong(),(double)time_f);
      return (((hashes_bn/time_f)/1000000)/1000).getuint256().getdouble();
    }
    blockindex = get_pprev_algo(blockindex,-1);
  } while (blockindex);
  return 0.;
}  

double GetMoneySupply (const CBlockIndex* blockindex, int algo) {
  if (blockindex == NULL)
    {
      if (chainActive.Tip() == NULL)
	return 0.;
      else
	blockindex = chainActive.Tip();
    }
  if (blockindex->nHeight == 0) {
    if (algo==-1) return 2.5;
    return 20.;
  }
  if (algo>=0) {
    int algo_tip = -1;
    if (onFork(blockindex)) {
      algo_tip = GetAlgo(blockindex->nVersion);
    }
    if (algo_tip != algo) {
      blockindex = get_pprev_algo(blockindex,algo);
    }
  }
  else {
    if (!onFork(blockindex)) {
      return ((double)blockindex->nMoneySupply)/100000000.;
    }
    return GetMoneySupply(blockindex,0)+GetMoneySupply(blockindex,1)+GetMoneySupply(blockindex,2)+GetMoneySupply(blockindex,3)+GetMoneySupply(blockindex,4)+GetMoneySupply(blockindex,5)+GetMoneySupply(blockindex,6)+GetMoneySupply(blockindex,7);
  }
  if (!blockindex) {
    blockindex = chainActive.Tip();
    while (blockindex && onFork(blockindex)) {
      blockindex = blockindex->pprev;
    }
    return ((double)GetMoneySupply(blockindex,-1))/8.;
  }
  if (blockindex->nMoneySupply == 0) return 2.5;
  return ((double)blockindex->nMoneySupply)/100000000.;
}

double GetBlockReward (CBlockIndex * blockindex, int algo, bool noScale) {
  if (blockindex == NULL) {
    if (chainActive.Tip() == NULL)
      return 0.;
    else
      blockindex = chainActive.Tip();
  }
  if (algo<0) {
    algo = GetAlgo(blockindex->nVersion);
  }
  auto_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
  if(!pblocktemplate.get())
    return 0.;
  CBlock *pblock = &pblocktemplate->block;
  pblock->nVersion = 4;
  pblock->SetAlgo(algo);
  CBlockIndex indexDummy(*pblock);
  indexDummy.pprev = blockindex;
  indexDummy.nHeight = blockindex->nHeight + 1;
  return ((double)GetBlockValue(&indexDummy,0,noScale))/100000000.;
  
}
  
int GetNBlocksUpdateSSF (const CBlockIndex * blockindex, const int algo) {
  if (blockindex == NULL) {
    if (chainActive.Tip() == NULL)
      return 0.;
    else
      blockindex = chainActive.Tip();
  }
  int algo_tip = -1;
  if (onFork(blockindex)) {
    algo_tip = GetAlgo(blockindex->nVersion);
  }
  if (algo>=0 && algo_tip != algo) {
    blockindex = get_pprev_algo(blockindex,algo);
  }
  if (!blockindex) return 0.;
  if (blockindex->nHeight == 0) return 0.;
  int n = nSSF;
  do {
    if (update_ssf(blockindex->nVersion)) {
      break;
    }
    blockindex = get_pprev_algo(blockindex,-1);
    n--;
  } while (blockindex);
  return n;
}

double GetAverageBlockSpacing (const CBlockIndex * blockindex, const int algo, const int averagingInterval) {
  
  if (averagingInterval <= 1) return 0.;

  if (blockindex == NULL) {
    if (chainActive.Tip() == NULL)
      return 0.;
    else
      blockindex = chainActive.Tip();
  }
  
  const CBlockIndex *BlockReading = blockindex;
  int64_t CountBlocks = 0;
  int64_t nActualTimespan = 0;
  int64_t LastBlockTime = 0;
  
  for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
    if (CountBlocks >= averagingInterval) { break; }
    int block_algo = -1;
    if (onFork(BlockReading)) {
      block_algo = GetAlgo(BlockReading->nVersion);
    }
    if (algo >=0 && block_algo != algo) {
      BlockReading = BlockReading->pprev;
      continue;
    }
    CountBlocks++;
    if(LastBlockTime > 0){
      int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
      nActualTimespan += Diff;
    }
    LastBlockTime = BlockReading->GetBlockTime();

    BlockReading = BlockReading->pprev;
    
  }
  return ((double)nActualTimespan)/((double)averagingInterval)/60.;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    result.push_back(Pair("powhash",block.GetPoWHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("coreversion",GetBlockVersion(block.nVersion)));
    int algo = GetAlgo(block.nVersion);
    result.push_back(Pair("algo",GetAlgoName(algo)));
    bool auxpow = block.IsAuxpow();
    result.push_back(Pair("auxpow",auxpow));
    if (auxpow) {
      result.push_back(Pair("parentblockhash",block.auxpow->parentBlock.GetHash().GetHex()));
      result.push_back(Pair("parentblockpowhash",block.auxpow->parentBlock.GetPoWHash().GetHex()));
      if (algo==ALGO_CRYPTONIGHT) {
	char prev_id [65];
	std::vector<unsigned char> vector_rep = block.auxpow->parentBlock.vector_rep;
	for (int i=0; i<32; i++) {
	  // 7 is the typical offset in monero, but not fully general
	  sprintf(prev_id+2*i,"%02x",vector_rep[i+7]);
	}
	result.push_back(Pair("parentblockprevhash",prev_id));
      }
      else {
	result.push_back(Pair("parentblockprevhash",block.auxpow->parentBlock.hashPrevBlock.GetHex()));
      }
    }
    result.push_back(Pair("SSF height",get_ssf_height(blockindex)));
    result.push_back(Pair("SSF work", (int64_t)get_ssf_work(blockindex)));
    result.push_back(Pair("SSF time",get_ssf_time(blockindex)));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    Array txs;
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
        txs.push_back(tx.GetHash().GetHex());
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex,algo,true,false)));
    result.push_back(Pair("chainwork", blockindex->nChainWork.GetHex()));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.push_back(Pair("nextblockhash", pnext->GetBlockHash().GetHex()));
    return result;
}


Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest block chain.\n"
            "\nResult:\n"
            "n    (numeric) The current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockcount", "")
            + HelpExampleRpc("getblockcount", "")
        );

    return chainActive.Height();
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "\nReturns the hash of the best (tip) block in the longest block chain.\n"
            "\nResult\n"
            "\"hex\"      (string) the block hash hex encoded\n"
            "\nExamples\n"
            + HelpExampleCli("getbestblockhash", "")
            + HelpExampleRpc("getbestblockhash", "")
        );

    return chainActive.Tip()->GetBlockHash().GetHex();
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getrawmempool ( verbose )\n"
            "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
            "\nArguments:\n"
            "1. verbose           (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
            "\nResult: (for verbose = false):\n"
            "[                     (json array of string)\n"
            "  \"transactionid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            "    \"size\" : n,             (numeric) transaction size in bytes\n"
            "    \"fee\" : n,              (numeric) transaction fee in bitmarks\n"
            "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
            "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
            "    \"startingpriority\" : n, (numeric) priority when transaction entered pool\n"
            "    \"currentpriority\" : n,  (numeric) transaction priority now\n"
            "    \"depends\" : [           (array) unconfirmed transactions used as inputs for this transaction\n"
            "        \"transactionid\",    (string) parent transaction id\n"
            "       ... ]\n"
            "  }, ...\n"
            "]\n"
            "\nExamples\n"
            + HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
        );

    bool fVerbose = false;
    if (params.size() > 0)
        fVerbose = params[0].get_bool();

    if (fVerbose)
    {
        LOCK(mempool.cs);
        Object o;
        BOOST_FOREACH(const PAIRTYPE(uint256, CTxMemPoolEntry)& entry, mempool.mapTx)
        {
            const uint256& hash = entry.first;
            const CTxMemPoolEntry& e = entry.second;
            Object info;
            info.push_back(Pair("size", (int)e.GetTxSize()));
            info.push_back(Pair("fee", ValueFromAmount(e.GetFee())));
            info.push_back(Pair("time", e.GetTime()));
            info.push_back(Pair("height", (int)e.GetHeight()));
            info.push_back(Pair("startingpriority", e.GetPriority(e.GetHeight())));
            info.push_back(Pair("currentpriority", e.GetPriority(chainActive.Height())));
            const CTransaction& tx = e.GetTx();
            set<string> setDepends;
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
            {
                if (mempool.exists(txin.prevout.hash))
                    setDepends.insert(txin.prevout.hash.ToString());
            }
            Array depends(setDepends.begin(), setDepends.end());
            info.push_back(Pair("depends", depends));
            o.push_back(Pair(hash.ToString(), info));
        }
        return o;
    }
    else
    {
        vector<uint256> vtxid;
        mempool.queryHashes(vtxid);

        Array a;
        BOOST_FOREACH(const uint256& hash, vtxid)
            a.push_back(hash.ToString());

        return a;
    }
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash index\n"
            "\nReturns hash of block in best-block-chain at index provided.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block index\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockhash", "1000")
            + HelpExampleRpc("getblockhash", "1000")
        );

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height())
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = chainActive[nHeight];
    return pblockindex->GetBlockHash().GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock \"hash\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1. \"hash\"          (string, required) The block hash\n"
            "2. verbose           (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
            "\nResult (for verbose = true):\n"
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n"
            + HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
            + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
        );

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    bool fVerbose = true;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    if(!ReadBlockFromDisk(block, pblockindex))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, pblockindex);
}

Value gettxoutsetinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "gettxoutsetinfo\n"
            "\nReturns statistics about the unspent transaction output set.\n"
            "Note this call may take some time.\n"
            "\nResult:\n"
            "{\n"
            "  \"height\":n,     (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",   (string) the best block hash hex\n"
            "  \"transactions\": n,      (numeric) The number of transactions\n"
            "  \"txouts\": n,            (numeric) The number of output transactions\n"
            "  \"bytes_serialized\": n,  (numeric) The serialized size\n"
            "  \"hash_serialized\": \"hash\",   (string) The serialized hash\n"
            "  \"total_amount\": x.xxx          (numeric) The total amount\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxoutsetinfo", "")
            + HelpExampleRpc("gettxoutsetinfo", "")
        );

    Object ret;

    CCoinsStats stats;
    if (pcoinsTip->GetStats(stats)) {
        ret.push_back(Pair("height", (int64_t)stats.nHeight));
        ret.push_back(Pair("bestblock", stats.hashBlock.GetHex()));
        ret.push_back(Pair("transactions", (int64_t)stats.nTransactions));
        ret.push_back(Pair("txouts", (int64_t)stats.nTransactionOutputs));
        ret.push_back(Pair("bytes_serialized", (int64_t)stats.nSerializedSize));
        ret.push_back(Pair("hash_serialized", stats.hashSerialized.GetHex()));
        ret.push_back(Pair("total_amount", ValueFromAmount(stats.nTotalAmount)));
    }
    return ret;
}

Value gettxout(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "gettxout \"txid\" n ( includemempool )\n"
            "\nReturns details about an unspent transaction output.\n"
            "\nArguments:\n"
            "1. \"txid\"       (string, required) The transaction id\n"
            "2. n              (numeric, required) vout value\n"
            "3. includemempool  (boolean, optional) Whether to included the mem pool\n"
            "\nResult:\n"
            "{\n"
            "  \"bestblock\" : \"hash\",    (string) the block hash\n"
            "  \"confirmations\" : n,       (numeric) The number of confirmations\n"
            "  \"value\" : x.xxx,           (numeric) The transaction value in btm\n"
            "  \"scriptPubKey\" : {         (json object)\n"
            "     \"asm\" : \"code\",       (string) \n"
            "     \"hex\" : \"hex\",        (string) \n"
            "     \"reqSigs\" : n,          (numeric) Number of required signatures\n"
            "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
            "     \"addresses\" : [          (array of string) array of bitmark addresses\n"
            "        \"bitmarkaddress\"     (string) bitmark address\n"
            "        ,...\n"
            "     ]\n"
            "  },\n"
            "  \"version\" : n,            (numeric) The version\n"
            "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
            "}\n"

            "\nExamples:\n"
            "\nGet unspent transactions\n"
            + HelpExampleCli("listunspent", "") +
            "\nView the details\n"
            + HelpExampleCli("gettxout", "\"txid\" 1") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("gettxout", "\"txid\", 1")
        );

    Object ret;

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);
    int n = params[1].get_int();
    bool fMempool = true;
    if (params.size() > 2)
        fMempool = params[2].get_bool();

    CCoins coins;
    if (fMempool) {
        LOCK(mempool.cs);
        CCoinsViewMemPool view(*pcoinsTip, mempool);
        if (!view.GetCoins(hash, coins))
            return Value::null;
        mempool.pruneSpent(hash, coins); // TODO: this should be done by the CCoinsViewMemPool
    } else {
        if (!pcoinsTip->GetCoins(hash, coins))
            return Value::null;
    }
    if (n<0 || (unsigned int)n>=coins.vout.size() || coins.vout[n].IsNull())
        return Value::null;

    std::map<uint256, CBlockIndex*>::iterator it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    CBlockIndex *pindex = it->second;
    ret.push_back(Pair("bestblock", pindex->GetBlockHash().GetHex()));
    if ((unsigned int)coins.nHeight == MEMPOOL_HEIGHT)
        ret.push_back(Pair("confirmations", 0));
    else
        ret.push_back(Pair("confirmations", pindex->nHeight - coins.nHeight + 1));
    ret.push_back(Pair("value", ValueFromAmount(coins.vout[n].nValue)));
    Object o;
    ScriptPubKeyToJSON(coins.vout[n].scriptPubKey, o, true);
    ret.push_back(Pair("scriptPubKey", o));
    ret.push_back(Pair("version", coins.nVersion));
    ret.push_back(Pair("coinbase", coins.fCoinBase));

    return ret;
}

Value verifychain(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "verifychain ( checklevel numblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1. checklevel   (numeric, optional, 0-4, default=3) How thorough the block verification is.\n"
            "2. numblocks    (numeric, optional, default=288, 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified or not\n"
            "\nExamples:\n"
            + HelpExampleCli("verifychain", "")
            + HelpExampleRpc("verifychain", "")
        );

    int nCheckLevel = GetArg("-checklevel", 3);
    int nCheckDepth = GetArg("-checkblocks", 288);
    if (params.size() > 0)
        nCheckLevel = params[0].get_int();
    if (params.size() > 1)
        nCheckDepth = params[1].get_int();

    return VerifyDB(nCheckLevel, nCheckDepth);
}

Value getblockchaininfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockchaininfo\n"
            "Returns an object containing various state info regarding block chain processing.\n"
            "\nResult:\n"
            "{\n"
            "  \"chain\": \"xxxx\",        (string) current chain (main, testnet3, regtest)\n"
            "  \"blocks\": xxxxxx,         (numeric) the current number of blocks processed in the server\n"
            "  \"bestblockhash\": \"...\", (string) the hash of the currently best block\n"
            "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
            "  \"chainwork\": \"xxxx\"     (string) total amount of work in active chain, in hexadecimal\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockchaininfo", "")
            + HelpExampleRpc("getblockchaininfo", "")
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    std::string chain = Params().DataDir();
    if(chain.empty())
        chain = "main";
    obj.push_back(Pair("chain",         chain));
    obj.push_back(Pair("blocks",        (int)chainActive.Height()));
    obj.push_back(Pair("bestblockhash", chainActive.Tip()->GetBlockHash().GetHex()));
    obj.push_back(Pair("difficulty",    (double)GetDifficulty(NULL,-1)));
    obj.push_back(Pair("verificationprogress", Checkpoints::GuessVerificationProgress(chainActive.Tip())));
    obj.push_back(Pair("chainwork",     chainActive.Tip()->nChainWork.GetHex()));
    return obj;
}
