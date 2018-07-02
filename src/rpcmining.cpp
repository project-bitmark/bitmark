// Copyright (c) 2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "chainparams.h"
#include "init.h"
#include "net.h"
#include "main.h"
#include "miner.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#endif
#include <stdint.h>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

#ifdef ENABLE_WALLET
// Key used by getwork miners.
// Allocated in InitRPCMining, free'd in ShutdownRPCMining
static CReserveKey* pMiningKey = NULL;

void InitRPCMining()
{
    if (!pwalletMain)
        return;

    // getwork/getblocktemplate mining rewards paid here:
    pMiningKey = new CReserveKey(pwalletMain);
}

void ShutdownRPCMining()
{
    if (!pMiningKey)
        return;

    delete pMiningKey; pMiningKey = NULL;
}
#else
void InitRPCMining()
{
}
void ShutdownRPCMining()
{
}
#endif

/* Set mining algo here for rpc mining */
int miningAlgo = ALGO_SCRYPT;
int miningAlgoGBT = miningAlgo;
int miningAlgoGAB = miningAlgo;
bool confAlgoIsSet = false;

// Return average network hashes per second based on the last 'lookup' blocks,
// or from the last difficulty change if 'lookup' is nonpositive.
// If 'height' is nonnegative, compute the estimate at the time when a given block was found.
Value GetNetworkHashPS(int lookup, int height, int algo) {
    CBlockIndex *pb = chainActive.Tip();

    if (height >= 0 && height < chainActive.Height())
        pb = chainActive[height];

    if (pb == NULL || !pb->nHeight)
        return 0;

    // If lookup is -1, then use blocks since last difficulty change.
    if (lookup <= 0)
        lookup = pb->nHeight % 2016 + 1;

    // If lookup is larger than chain, then set it to chain length.
    if (lookup > pb->nHeight)
        lookup = pb->nHeight;

    CBlockIndex *pb0 = pb;
    int algo_tip = GetAlgo(pb0->nVersion);
    if (algo_tip != algo) {
      pb0 = get_pprev_algo(pb0,algo);
    }
    if (!pb0) return 0.;
    int64_t minTime = pb0->GetBlockTime();
    int64_t maxTime = minTime;
    CBigNum hashes_bn = pb0->GetBlockWork();
    for (int i = 0; i < lookup; i++) {
        pb0 = pb0->pprev;
	if (!pb0) break;
	if (GetAlgo(pb0->nVersion)!=algo) {
	  lookup++;
	  continue;
	}
        int64_t time = pb0->GetBlockTime();
        minTime = std::min(time, minTime);
        maxTime = std::max(time, maxTime);
	hashes_bn += pb0->GetBlockWork();
    }

    // In case there's a situation where minTime == maxTime, we don't want a divide by zero exception.
    if (minTime == maxTime)
        return 0;

    //uint256 workDiff = pb->nChainWork - pb0->nChainWork;
    int64_t timeDiff = maxTime - minTime;

    return (((double)hashes_bn.getulong()) / (double)timeDiff);
}

Value getnetworkhashps(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
            "getnetworkhashps ( blocks height )\n"
            "\nReturns the estimated network hashes per second based on the last n blocks.\n"
            "Pass in [blocks] to override # of blocks, -1 specifies since last difficulty change.\n"
            "Pass in [height] to estimate the network speed at the time when a certain block was found.\n"
            "\nArguments:\n"
            "1. blocks     (numeric, optional, default=120) The number of blocks, or -1 for blocks since last difficulty change.\n"
            "2. height     (numeric, optional, default=-1) To estimate at the time of the given height.\n"
            "\nResult:\n"
            "x             (numeric) Hashes per second estimated\n"
            "\nExamples:\n"
            + HelpExampleCli("getnetworkhashps", "")
            + HelpExampleRpc("getnetworkhashps", "")
       );

    return GetNetworkHashPS(params.size() > 0 ? params[0].get_int() : 120, params.size() > 1 ? params[1].get_int() : -1, params.size() > 2 ? params[2].get_int() : ALGO_SCRYPT);
}

#ifdef ENABLE_WALLET
Value getgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getgenerate\n"
            "\nReturn if the server is set to generate coins or not. The default is false.\n"
            "It is set with the command line argument -gen (or bitmark.conf setting gen)\n"
            "It can also be set with the setgenerate call.\n"
            "\nResult\n"
            "true|false      (boolean) If the server is set to generate coins or not\n"
            "\nExamples:\n"
            + HelpExampleCli("getgenerate", "")
            + HelpExampleRpc("getgenerate", "")
        );

    if (!pMiningKey)
        return false;

    return GetBoolArg("-gen", false);
}

Value setminingalgo(const Array& params, bool fHelp)
{
  if (fHelp || params.size() != 1)
    throw runtime_error("setminingalgo algo"
			"\nSet the algorithm for mining purposes\n"
			"\nArguments:\n"
			"1. algo         (numeric, required).\n"
			);

  miningAlgo = params[0].get_int();

  return Value::null;
}

Value getminingalgo(const Array& params, bool fHelp)
{
  if (fHelp || params.size() != 0)
    throw runtime_error("getminingalgo"
			"\nGet the mining algorithm\n"
			"\nResult\n"
			"x (1-5) The number representing the mining algorithm\n"			
			);

  if (!confAlgoIsSet) {
    miningAlgo = GetArg("-miningalgo", miningAlgo);
    confAlgoIsSet = true;
  }
  return (int)miningAlgo;
}
      

Value setgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "setgenerate generate ( genproclimit )\n"
            "\nSet 'generate' true or false to turn generation on or off.\n"
            "Generation is limited to 'genproclimit' processors, -1 is unlimited.\n"
            "See the getgenerate call for the current setting.\n"
            "\nArguments:\n"
            "1. generate         (boolean, required) Set to true to turn on generation, off to turn off.\n"
            "2. genproclimit     (numeric, optional) Set the processor limit for when generation is on. Can be -1 for unlimited.\n"
	    "3. mining algo (numeric, optional) Set the mining algo"
            "                    Note: in -regtest mode, genproclimit controls how many blocks are generated immediately.\n"
            "\nExamples:\n"
            "\nSet the generation on with a limit of one processor and SHA256D for the algo\n"
            + HelpExampleCli("setgenerate", "true 1 1") +
            "\nCheck the setting\n"
            + HelpExampleCli("getgenerate", "") +
            "\nTurn off generation\n"
            + HelpExampleCli("setgenerate", "false") +
            "\nUsing json rpc\n"
            + HelpExampleRpc("setgenerate", "true, 1")
        );

    if (pwalletMain == NULL)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");

    bool fGenerate = true;
    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    int nGenProcLimit = -1;
    if (params.size() > 1)
    {
        nGenProcLimit = params[1].get_int();
        if (nGenProcLimit == 0)
            fGenerate = false;
    }

    if (params.size() > 2) {
      miningAlgo = params[2].get_int();
    }
    
    // -regtest mode: don't return until nGenProcLimit blocks are generated
    if (fGenerate && Params().NetworkID() == CChainParams::REGTEST)
    {
        int nHeightStart = 0;
        int nHeightEnd = 0;
        int nHeight = 0;
        int nGenerate = (nGenProcLimit > 0 ? nGenProcLimit : 1);
        {   // Don't keep cs_main locked
            LOCK(cs_main);
            nHeightStart = chainActive.Height();
            nHeight = nHeightStart;
            nHeightEnd = nHeightStart+nGenerate;
        }
        int nHeightLast = -1;
        while (nHeight < nHeightEnd)
        {
            if (nHeightLast != nHeight)
            {
                nHeightLast = nHeight;
                GenerateBitmarks(fGenerate, pwalletMain, 1);
            }
            MilliSleep(1);
            {   // Don't keep cs_main locked
                LOCK(cs_main);
                nHeight = chainActive.Height();
            }
        }
    }
    else // Not -regtest: start generate thread, return immediately
    {
        mapArgs["-gen"] = (fGenerate ? "1" : "0");
        mapArgs ["-genproclimit"] = itostr(nGenProcLimit);
	LogPrintf("do generatebitmarks\n");
        GenerateBitmarks(fGenerate, pwalletMain, nGenProcLimit);
    }

    return Value::null;
}

Value gethashespersec(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "gethashespersec\n"
            "\nReturns a recent hashes per second performance measurement while generating.\n"
            "See the getgenerate and setgenerate calls to turn generation on and off.\n"
            "\nResult:\n"
            "n            (numeric) The recent hashes per second when generation is on (will return 0 if generation is off)\n"
            "\nExamples:\n"
            + HelpExampleCli("gethashespersec", "")
            + HelpExampleRpc("gethashespersec", "")
        );

    if (GetTimeMillis() - nHPSTimerStart > 8000)
        return (int64_t)0;
    return (int64_t)dHashesPerSec;
}
#endif


Value getmininginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getmininginfo\n"
            "\nReturns a json object containing mining-related information."
            "\nResult:\n"
            "{\n"
            "  \"blocks\": nnn,             (numeric) The current block\n"
            "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
            "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"
            "  \"reward_next\": nnn,        (numeric) The next block reward\n"
            "  \"reward_max\": nnn,         (numeric) The maximum block reward\n"
            "  \"hashrate_4max_reward\": nnn, (numeric) The hashrate required for max reward\n"
            "  \"pow_algo_id\": n           (numeric) The active mining algorithm id\n"
            "  \"pow_algo\": \"name\"       (string) The active mining algorithm name\n"
            "  \"difficulty\": xxx.xxxxx    (numeric) The current (weighted) difficulty\n"
//		unweighted "simple" difficulty.
            "  \"sdifficulty\": xxx.xxxxx    (numeric) The current (unweighted) difficulty\n"
            "  \"difficulty_scrypt\": xxxxxx,   (numeric) the current scrypt difficulty\n"
            "  \"difficulty_sha256d\": xxxxxx,  (numeric) the current sha256d difficulty\n"
            "  \"difficulty_yescrypt\": xxxxxx, (numeric) the current yescrypt difficulty\n"
            "  \"difficulty_argon2d\": xxxxxx,    (numeric) the current argon2d difficulty\n"
            "  \"difficulty_x17\": xxxxxx,    (numeric) the current x17 difficulty\n"
            "  \"difficulty_lyra2rev2\": xxxxxx,    (numeric) the current lyra2rev2 difficulty\n"
            "  \"difficulty_equihash\": xxxxxx,  (numeric) the current equihash difficulty\n"
            "  \"difficulty_cryptonight\": xxxxxx,  (numeric) the current cryptonight difficulty\n"
            "  \"errors\": \"...\"          (string) Current errors\n"
            "  \"generate\": true|false     (boolean) If the generation is on or off (see getgenerate or setgenerate calls)\n"
            "  \"genproclimit\": n          (numeric) The processor limit for generation. -1 if no generation. (see getgenerate or setgenerate calls)\n"
            "  \"hashespersec\": n          (numeric) The hashes per second of the generation, or 0 if no generation.\n"
            "  \"pooledtx\": n              (numeric) The size of the mem pool\n"
            "  \"testnet\": true|false      (boolean) If using testnet or not\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
        );

    Object obj;
    obj.push_back(Pair("blocks",           (int)chainActive.Height()));
    obj.push_back(Pair("currentblocksize", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",   (uint64_t)nLastBlockTx));
    obj.push_back(Pair("reward_next",      ValueFromAmount(GetBlockReward(chainActive.Tip(),miningAlgo,false)*100000000.)));
    obj.push_back(Pair("reward_max",       ValueFromAmount(GetBlockReward(chainActive.Tip(),miningAlgo,true)*100000000.)));
    obj.push_back(Pair("pow_algo_id", miningAlgo));
    obj.push_back(Pair("pow_algo",GetAlgoName(miningAlgo)));
    //obj.push_back(Pair("hashrate_4max_reward", (uint64_t)35000000000));
//  difficulty is weighted in Bitmark to more meaningfully compare relative values of competing chains
//                                       boolean for weighted / unweighted -------v
    obj.push_back(Pair("difficulty",      (double)GetDifficulty(NULL,miningAlgo,true,true)));
//  sdifficulty: the "simple", unweighted difficulty
    obj.push_back(Pair("sdifficulty",       (double)GetDifficulty(NULL,miningAlgo,false,true)));
    obj.push_back(Pair("difficulty SCRYPT", (double)GetDifficulty(NULL,ALGO_SCRYPT,true,true)));
    obj.push_back(Pair("difficulty SHA256D",    (double)GetDifficulty(NULL,ALGO_SHA256D,true,true)));
    obj.push_back(Pair("difficulty YESCRYPT",    (double)GetDifficulty(NULL,ALGO_YESCRYPT,true,true)));
    obj.push_back(Pair("difficulty ARGON2",    (double)GetDifficulty(NULL,ALGO_ARGON2,true,true)));
    obj.push_back(Pair("difficulty X17",    (double)GetDifficulty(NULL,ALGO_X17,true,true)));
    obj.push_back(Pair("difficulty LYRA2REv2",    (double)GetDifficulty(NULL,ALGO_LYRA2REv2,true,true)));
    obj.push_back(Pair("difficulty EQUIHASH",    (double)GetDifficulty(NULL,ALGO_EQUIHASH,true,true)));
    obj.push_back(Pair("difficulty CRYPTONIGHT",    (double)GetDifficulty(NULL,ALGO_CRYPTONIGHT,true,true)));
    obj.push_back(Pair("errors",           GetWarnings("statusbar")));
    obj.push_back(Pair("genproclimit",     (int)GetArg("-genproclimit", -1)));
    obj.push_back(Pair("networkhashps",    getnetworkhashps(params, false)));
    obj.push_back(Pair("pooledtx",         (uint64_t)mempool.size()));
    obj.push_back(Pair("testnet",          TestNet()));
#ifdef ENABLE_WALLET
    obj.push_back(Pair("generate",         getgenerate(params, false)));
    obj.push_back(Pair("hashespersec",     gethashespersec(params, false)));
#endif
    return obj;
}


#ifdef ENABLE_WALLET
Value getwork(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getwork ( \"data\" )\n"
            "\nIf 'data' is not specified, it returns the formatted hash data to work on.\n"
            "If 'data' is specified, tries to solve the block and returns true if it was successful.\n"
            "\nArguments:\n"
            "1. \"data\"       (string, optional) The hex encoded data to solve\n"
            "\nResult (when 'data' is not specified):\n"
            "{\n"
            "  \"midstate\" : \"xxxx\",   (string) The precomputed hash state after hashing the first half of the data (DEPRECATED)\n" // deprecated
            "  \"data\" : \"xxxxx\",      (string) The block data\n"
            "  \"hash1\" : \"xxxxx\",     (string) The formatted hash buffer for second hash (DEPRECATED)\n" // deprecated
            "  \"target\" : \"xxxx\"      (string) The little endian hash target\n"
            "}\n"
            "\nResult (when 'data' is specified):\n"
            "true|false       (boolean) If solving the block specified in the 'data' was successfull\n"
            "\nExamples:\n"
            + HelpExampleCli("getwork", "")
            + HelpExampleRpc("getwork", "")
        );

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Bitmark is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Bitmark is downloading blocks...");

    typedef map<uint256, pair<CBlock*, CScript> > mapNewBlock_t;
    static mapNewBlock_t mapNewBlock;    // FIXME: thread safety
    static vector<CBlockTemplate*> vNewBlockTemplate;

    if (params.size() == 0)
    {
        // Update block
        static unsigned int nTransactionsUpdatedLast;
        static CBlockIndex* pindexPrev;
        static int64_t nStart;
        static CBlockTemplate* pblocktemplate;
        if (pindexPrev != chainActive.Tip() ||
            (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60))
        {
            if (pindexPrev != chainActive.Tip())
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                BOOST_FOREACH(CBlockTemplate* pblocktemplate, vNewBlockTemplate)
                    delete pblocktemplate;
                vNewBlockTemplate.clear();
            }

            // Clear pindexPrev so future getworks make a new block, despite any failures from here on
            pindexPrev = NULL;

            // Store the pindexBest used before CreateNewBlock, to avoid races
            nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrevNew = chainActive.Tip();
            nStart = GetTime();

            // Create new block
            pblocktemplate = CreateNewBlockWithKey(*pMiningKey);
            if (!pblocktemplate)
                throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
            vNewBlockTemplate.push_back(pblocktemplate);

            // Need to update only after we know CreateNewBlock succeeded
            pindexPrev = pindexPrevNew;
        }
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience

	if ((pindexPrev->nHeight >= nForkHeight - 1 && CBlockIndex::IsSuperMajority(4,pindexPrev,75,100))) {
	  //pblock->nVersion = 3;
	  pblock->SetAlgo(miningAlgo);
	}

        // Update nTime
        UpdateTime(*pblock, pindexPrev);
        pblock->nNonce = 0;

        // Update nExtraNonce
        static unsigned int nExtraNonce = 0;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

        // Save
        mapNewBlock[pblock->hashMerkleRoot] = make_pair(pblock, pblock->vtx[0].vin[0].scriptSig);

        // Pre-build hash buffers
        char pmidstate[32];
        char pdata[128];
        char phash1[64];
        FormatHashBuffers(pblock, pmidstate, pdata, phash1);

        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

        Object result;
        result.push_back(Pair("midstate", HexStr(BEGIN(pmidstate), END(pmidstate)))); // deprecated
        result.push_back(Pair("data",     HexStr(BEGIN(pdata), END(pdata))));
        result.push_back(Pair("hash1",    HexStr(BEGIN(phash1), END(phash1)))); // deprecated
        result.push_back(Pair("target",   HexStr(BEGIN(hashTarget), END(hashTarget))));
        return result;
    }
    else
    {
        // Parse parameters
        vector<unsigned char> vchData = ParseHex(params[0].get_str());
        if (vchData.size() != 128)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
        CBlock* pdata = (CBlock*)&vchData[0];

        // Byte reverse
        for (int i = 0; i < 128/4; i++)
            ((unsigned int*)pdata)[i] = ByteReverse(((unsigned int*)pdata)[i]);

        // Get saved block
        if (!mapNewBlock.count(pdata->hashMerkleRoot))
            return false;
        CBlock* pblock = mapNewBlock[pdata->hashMerkleRoot].first;

        pblock->nTime = pdata->nTime;
        pblock->nNonce = pdata->nNonce;
        pblock->vtx[0].vin[0].scriptSig = mapNewBlock[pdata->hashMerkleRoot].second;
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        assert(pwalletMain != NULL);
        return CheckWork(pblock, *pwalletMain, *pMiningKey);
    }
}
#endif

Value getblocktemplate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getblocktemplate ( \"jsonrequestobject\" )\n"
            "\nIf the request parameters include a 'mode' key, that is used to explicitly select between the default 'template' request or a 'proposal'.\n"
            "It returns data needed to construct a block to work on.\n"
            "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

            "\nArguments:\n"
            "1. \"jsonrequestobject\"       (string, optional) A json object in the following spec\n"
            "     {\n"
            "       \"mode\":\"template\"    (string, optional) This must be set to \"template\" or omitted\n"
            "       \"capabilities\":[       (array, optional) A list of strings\n"
            "           \"support\"           (string) client side supported feature, 'longpoll', 'coinbasetxn', 'coinbasevalue', 'proposal', 'serverlist', 'workid'\n"
            "           ,...\n"
            "         ]\n"
            "     }\n"
            "\n"

            "\nResult:\n"
            "{\n"
            "  \"version\" : n,                    (numeric) The block version\n"
            "  \"previousblockhash\" : \"xxxx\",    (string) The hash of current highest block\n"
            "  \"transactions\" : [                (array) contents of non-coinbase transactions that should be included in the next block\n"
            "      {\n"
            "         \"data\" : \"xxxx\",          (string) transaction data encoded in hexadecimal (byte-for-byte)\n"
            "         \"hash\" : \"xxxx\",          (string) hash/id encoded in little-endian hexadecimal\n"
            "         \"depends\" : [              (array) array of numbers \n"
            "             n                        (numeric) transactions before this one (by 1-based index in 'transactions' list) that must be present in the final block if this one is\n"
            "             ,...\n"
            "         ],\n"
            "         \"fee\": n,                   (numeric) difference in value between transaction inputs and outputs (in Satoshis); for coinbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one\n"
            "         \"sigops\" : n,               (numeric) total number of SigOps, as counted for purposes of block limits; if key is not present, sigop count is unknown and clients MUST NOT assume there aren't any\n"
            "         \"required\" : true|false     (boolean) if provided and true, this transaction must be in the final block\n"
            "      }\n"
            "      ,...\n"
            "  ],\n"
            "  \"coinbaseaux\" : {                  (json object) data that should be included in the coinbase's scriptSig content\n"
            "      \"flags\" : \"flags\"            (string) \n"
            "  },\n"
            "  \"coinbasevalue\" : n,               (numeric) maximum allowable input to coinbase transaction, including the generation award and transaction fees (in Satoshis)\n"
            "  \"coinbasetxn\" : { ... },           (json object) information for coinbase transaction\n"
            "  \"target\" : \"xxxx\",               (string) The hash target\n"
            "  \"mintime\" : xxx,                   (numeric) The minimum timestamp appropriate for next block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mutable\" : [                      (array of string) list of ways the block template may be changed \n"
            "     \"value\"                         (string) A way the block template may be changed, e.g. 'time', 'transactions', 'prevblock'\n"
            "     ,...\n"
            "  ],\n"
            "  \"noncerange\" : \"00000000ffffffff\",   (string) A range of valid nonces\n"
            "  \"sigoplimit\" : n,                 (numeric) limit of sigops in blocks\n"
            "  \"sizelimit\" : n,                  (numeric) limit of block size\n"
            "  \"curtime\" : ttt,                  (numeric) current timestamp in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"bits\" : \"xxx\",                 (string) compressed target of next block\n"
            "  \"height\" : n                      (numeric) The height of the next block\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getblocktemplate", "")
            + HelpExampleRpc("getblocktemplate", "")
         );

    std::string strMode = "template";
    if (params.size() > 0)
    {
        const Object& oparam = params[0].get_obj();
        const Value& modeval = find_value(oparam, "mode");
        if (modeval.type() == str_type)
            strMode = modeval.get_str();
        else if (modeval.type() == null_type)
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Bitmark is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Bitmark is downloading blocks...");

    // Update block
    static unsigned int nTransactionsUpdatedLast;
    static CBlockIndex* pindexPrev;
    static int64_t nStart;
    static CBlockTemplate* pblocktemplate;
    if (pindexPrev != chainActive.Tip() || miningAlgo != miningAlgoGBT ||
        (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = NULL;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrevNew = chainActive.Tip();
        nStart = GetTime();

        // Create new block
        if(pblocktemplate)
        {
            delete pblocktemplate;
            pblocktemplate = NULL;
        }
        CScript scriptDummy = CScript() << OP_TRUE;
        pblocktemplate = CreateNewBlock(scriptDummy);
        if (!pblocktemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
	miningAlgoGBT = miningAlgo;
    }
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // Update nTime
    UpdateTime(*pblock, pindexPrev);
    pblock->nNonce = 0;

    Array transactions;
    map<uint256, int64_t> setTxIndex;
    int i = 0;
    BOOST_FOREACH (CTransaction& tx, pblock->vtx)
    {
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        Object entry;

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;
        entry.push_back(Pair("data", HexStr(ssTx.begin(), ssTx.end())));

        entry.push_back(Pair("hash", txHash.GetHex()));

        Array deps;
        BOOST_FOREACH (const CTxIn &in, tx.vin)
        {
            if (setTxIndex.count(in.prevout.hash))
                deps.push_back(setTxIndex[in.prevout.hash]);
        }
        entry.push_back(Pair("depends", deps));

        int index_in_template = i - 1;
        entry.push_back(Pair("fee", pblocktemplate->vTxFees[index_in_template]));
        entry.push_back(Pair("sigops", pblocktemplate->vTxSigOps[index_in_template]));

        transactions.push_back(entry);
    }

    Object aux;
    aux.push_back(Pair("flags", HexStr(COINBASE_FLAGS.begin(), COINBASE_FLAGS.end())));

    uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

    static Array aMutable;
    if (aMutable.empty())
    {
        aMutable.push_back("time");
        aMutable.push_back("transactions");
        aMutable.push_back("prevblock");
    }

    Object result;
    result.push_back(Pair("version", pblock->nVersion));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("transactions", transactions));
    result.push_back(Pair("coinbaseaux", aux));
    result.push_back(Pair("coinbasevalue", (int64_t)pblock->vtx[0].vout[0].nValue));
    result.push_back(Pair("target", hashTarget.GetHex()));
    result.push_back(Pair("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1));
    result.push_back(Pair("mutable", aMutable));
    result.push_back(Pair("noncerange", "00000000ffffffff"));
    result.push_back(Pair("sigoplimit", (int64_t)MAX_BLOCK_SIGOPS));
    result.push_back(Pair("sizelimit", (int64_t)MAX_BLOCK_SIZE));
    result.push_back(Pair("curtime", (int64_t)pblock->nTime));
    result.push_back(Pair("bits", HexBits(pblock->nBits)));
    result.push_back(Pair("height", (int64_t)(pindexPrev->nHeight+1)));

    return result;
}

Value submitblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "submitblock \"hexdata\" ( \"jsonparametersobject\" )\n"
            "\nAttempts to submit new block to network.\n"
            "The 'jsonparametersobject' parameter is currently ignored.\n"
            "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

            "\nArguments\n"
            "1. \"hexdata\"    (string, required) the hex-encoded block data to submit\n"
            "2. \"jsonparametersobject\"     (string, optional) object of optional parameters\n"
            "    {\n"
            "      \"workid\" : \"id\"    (string, optional) if the server provided a workid, it MUST be included with submissions\n"
            "    }\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitblock", "\"mydata\"")
            + HelpExampleRpc("submitblock", "\"mydata\"")
        );

    vector<unsigned char> blockData(ParseHex(params[0].get_str()));
    LogPrintf("block submitted:\n%s\n",params[0].get_str());
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    CBlock pblock;
    try {
        ssBlock >> pblock;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    LogPrintf("block algo is %d\n",pblock.GetAlgo());

    CValidationState state;
    bool fAccepted = ProcessBlock(state, NULL, &pblock);
    if (!fAccepted)
        return "rejected"; // TODO: report validation state

    return Value::null;
}

Value getauxblock(const Array& params, bool fHelp)
{
  if (fHelp || (params.size() != 0 && params.size() != 2))
    throw runtime_error(
			"getauxblock (hash auxpow)\n"
	                "\nCreate or submit a merge-mined block.\n"
	                "\nWithout arguments, create a new block and return information\n"
	                "required to merge-mine it.  With arguments, submit a solved\n"
	                "auxpow for a previously returned block.\n"
	                "\nArguments:\n"
	                "1. \"hash\"    (string, optional) hash of the block to submit\n"
	                "2. \"auxpow\"  (string, optional) serialised auxpow found\n"
	                "\nResult (without arguments):\n"
	                "{\n"
	                "  \"hash\"               (string) hash of the created block\n"
	                "  \"chainid\"            (numeric) chain ID for this block\n"
	                "  \"previousblockhash\"  (string) hash of the previous block\n"
	                "  \"coinbasevalue\"      (numeric) value of the block's coinbase\n"
	                "  \"bits\"               (string) compressed target of the block\n"
	                "  \"height\"             (numeric) height of the block\n"
	                "  \"target\"             (string) target in reversed byte order\n"
			"{\n"
			"  \"hash\"               (string) hash of the created block\n"
			"  \"chainid\"            (numeric) chain ID for this block\n"
			"  \"previousblockhash\"  (string) hash of the previous block\n"
			"  \"coinbasevalue\"      (numeric) value of the block's coinbase\n"
			"  \"bits\"               (string) compressed target of the block\n"
			"  \"height\"             (numeric) height of the block\n"
			"  \"target\"             (string) target in reversed byte order\n"
			"}\n"
			"\nResult (with arguments):\n"
			"xxxxx        (boolean) whether the submitted block was correct\n"
			"\nExamples:\n"
			+ HelpExampleCli("getauxblock", "")
			+ HelpExampleCli("getauxblock", "\"hash\" \"serialised auxpow\"")
			+ HelpExampleRpc("getauxblock", "")
			);
  if (pwalletMain == NULL)
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");
  if (vNodes.empty())
    throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Bitmark is not connected!");
  if (IsInitialBlockDownload())
    throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Bitmark is downloading blocks...");
  static CCriticalSection cs_auxblockCache;
  LOCK(cs_auxblockCache);
  static std::map<uint256, CBlock*> mapNewBlock;
  static std::vector<CBlockTemplate*> vNewBlockTemplate;
  if (params.size() == 0) {
    static unsigned nTransactionsUpdatedLast;
    static CBlockIndex* pindexPrev = NULL;
    static uint64_t nStart;
    static CBlockTemplate* pblocktemplate;
    static unsigned int nExtraNonce = 0;
    CReserveKey reservekey(pwalletMain);

    {
      LOCK(cs_main);
      if (pindexPrev != chainActive.Tip() || miningAlgo != miningAlgoGAB
	  || (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast
	      && GetTime() - nStart > 60)) {
	if (pindexPrev != chainActive.Tip()) {
	  mapNewBlock.clear();
	  BOOST_FOREACH(CBlockTemplate* pbt, vNewBlockTemplate)
	    delete pbt;
	  vNewBlockTemplate.clear();
	}

	pblocktemplate = CreateNewBlockWithKey(reservekey);
	if (!pblocktemplate)
	  throw JSONRPCError(RPC_OUT_OF_MEMORY, "out of memory");

	nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
	pindexPrev = chainActive.Tip();
	nStart = GetTime();

	CBlock* pblock = &pblocktemplate->block;
	IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
	pblock->SetAuxpow(true);
	pblock->SetChainId(Params().GetAuxpowChainId());
	pblock->hashMerkleRoot = pblock->BuildMerkleTree();

	mapNewBlock[pblock->GetHash()] = pblock;
	vNewBlockTemplate.push_back(pblocktemplate);
	miningAlgoGAB = miningAlgo;
      }	    	  
    }

    const CBlock& block = pblocktemplate->block;

    uint256 hashTarget = CBigNum().SetCompact(block.nBits).getuint256();

    json_spirit::Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    result.push_back(Pair("chainid", block.GetChainId()));
    result.push_back(Pair("previousblockhash", block.hashPrevBlock.GetHex()));
    result.push_back(Pair("coinbasevalue", (int64_t)block.vtx[0].vout[0].nValue));
    /*
    char * coinbasedata = (char *)malloc(2*174+1);
    for (int i=0; i<174; i++) {
      sprintf(coinbasedata+2*i,"%02x",((unsigned char *)&block.vtx[0])[i]);
    }
    result.push_back(Pair("coinbasedata",coinbasedata));
    */
    //block.vtx[0].print();
    CTxDestination address;
    ExtractDestination(block.vtx[0].vout[0].scriptPubKey,address);
    result.push_back(Pair("address",CBitmarkAddress(address).ToString()));
    result.push_back(Pair("bits", strprintf("%08x", block.nBits)));
    result.push_back(Pair("height", static_cast<int64_t> (pindexPrev->nHeight + 1)));
    result.push_back(Pair("target", HexStr(BEGIN(hashTarget), END(hashTarget))));
    result.push_back(Pair("version",block.nVersion));
    result.push_back(Pair("curtime", (int64_t)block.nTime));
    result.push_back(Pair("scriptsig",HexStr(block.vtx[0].vin[0].scriptSig)));

    return result;
  }
  assert(params.size() == 2);
  uint256 hash;
  const char * hash_str = params[0].get_str().c_str();
  if (fDebug) LogPrintf("getauxblock hash_str = %s\n",hash_str);
  hash.SetHex(params[0].get_str());
  const std::map<uint256, CBlock*>::iterator mit = mapNewBlock.find(hash);
  if (strlen(hash_str)>0 && mit == mapNewBlock.end())
    throw JSONRPCError(RPC_INVALID_PARAMETER, "block hash unknown");
  CBlock& block = *mit->second;
  const char * block_str = params[1].get_str().c_str();
  if (fDebug) LogPrintf("getauxblock block_str = %s\n",block_str);
  const std::vector<unsigned char> vchAuxPow = ParseHex(params[1].get_str());
  CDataStream ss(vchAuxPow, SER_GETHASH, PROTOCOL_VERSION);
  CAuxPow pow;
  if (block.GetAlgo()==ALGO_EQUIHASH || block.GetAlgo()==ALGO_CRYPTONIGHT) {
    pow.vector_format = true;
  }
  if (block.GetAlgo()==ALGO_CRYPTONIGHT) {
    pow.parentBlock.vector_format = true;
    pow.keccak_hash = true;
  }
  pow.parentBlock.algoParent = block.GetAlgo();
  pow.parentBlock.isParent = true;
  ss >> pow;
  block.SetAuxpow(new CAuxPow(pow));
  if (strlen(hash_str)>0) {
    assert(block.GetHash() == hash);
  }
  CValidationState state;
  bool fAccepted = ProcessBlock(state, NULL, &block);
  if (!fAccepted)
    return "rejected";
  return Value::null;
}
