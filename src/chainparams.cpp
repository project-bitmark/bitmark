// Copyright (c) 2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014-2018 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//

// Hard-Coded (Fixed) Network Nodes
unsigned int pnSeed[] =
{
	0xADFFFC8C,		// seed.bitmark.co	IP = 173.255.252.140	sam
        0x8BA2805C,             // eu.bitmark.io        IP = 139.162.128.92     frank
        0x5E89B1E3,             // ge.bitmark.io        IP = 94.137.177.227     anton
        0x8BA27A8A,             // jp.bitmark.io        IP = 139.162.122.138    akio
        0x2D2141A1,             // us.bitmark.io        IP = 45.33.65.161       joe
	0x8BA223AA,		// sg.bitmark.co	IP = 139.162.35.170     ben
};

unsigned int pnSeedTest[] =
  {
    0x5E172153,
  };

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        vAlertPubKey = ParseHex("04bf5a75ff0f823840ef512b08add20bb4275ff6e097f2830ad28645e28cb5ea4dc2cfd0972b94019ad46f331b45ef4ba679f2e6c87fd19c864365fadb4f8d2269");
        nDefaultPort = 9265;
        nRPCPort = 9266;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 32);
        nSubsidyHalvingInterval = 788000;
	fStrictChainId = true;
	nAuxpowChainId = 0x005B;
	nEquihashN = 200;
	nEquihashK = 9;
	fMineBlocksOnDemand = false;

        // Build the genesis block.
        const char* pszTimestamp = "13/July/2014, with memory of the past, we look to the future. TDR";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 20 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04f88a76429dad346a10ecb5d36fcbf50bc2e009870e20c1a6df8db743e0b994afc1f91e079be8acc380b0ee7765519906e3d781519e9db48259f64160104939d8") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1405274442;
        genesis.nBits    = 0x1d00ffff;
        genesis.nNonce   = 14385103;

        hashGenesisBlock = genesis.GetHash();	
	
        assert(hashGenesisBlock == uint256("0xc1fb746e87e89ae75bdec2ef0639a1f6786744639ce3d0ece1dcf979b79137cb"));
        assert(genesis.hashMerkleRoot == uint256("0xd4715adf41222fae3d4bf41af30c675bc27228233d0f3cfd4ae0ae1d3e760ba8"));

        // DNS Seeders - Verified       03 08 21   March 8, 2021 
        //                               Domain                  Sub-Domain        Location   
        vSeeds.push_back(CDNSSeedData("bitmark.guru",        "da.bitmark.guru"));    // DE Frank 
        vSeeds.push_back(CDNSSeedData("openmarks.com",     "btm.openmarks.com"));    // IL eli  
        vSeeds.push_back(CDNSSeedData("bitmark.one",       "shido.bitmark.one"));    // JP akio 
 
        vSeeds.push_back(CDNSSeedData("openmarks.com", "dnsseed.openmarks.com"));    // DE omar  
        vSeeds.push_back(CDNSSeedData("zmark.org",              "ra.zmark.org"));    // CA sam 
        vSeeds.push_back(CDNSSeedData("chainetics.com", "marks.chainetics.com"));    // SG ben 
 
        vSeeds.push_back(CDNSSeedData("bitmark.one",        "biji.bitmark.one"));    // CA marks  
        vSeeds.push_back(CDNSSeedData("bitmark.cc",       "dnsseed.bitmark.cc"));    // NJ joe 
        vSeeds.push_back(CDNSSeedData("zmark.org",            "btmk.zmark.org"));    // CA zappa 

        vSeeds.push_back(CDNSSeedData("zmark.org",           "shiba.zmark.org"));    // NJ j2     
        vSeeds.push_back(CDNSSeedData("avalax.com",         "marks.avalax.com"));    // JP jin 
        vSeeds.push_back(CDNSSeedData("zmark.org",             "btm.zmark.org"));    // NJ vinny j0     

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,85); // b
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =    std::vector<unsigned char>(1,213);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // Convert the pnSeeds array into usable address objects.
        for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
        {
            // It'll only connect to one or two seed nodes because once it connects,
            // it'll get a pile of addresses with newer timestamps.
            // Seed nodes are given a random 'last seen time' of between one and two
            // weeks ago.
            const int64_t nOneWeek = 7*24*60*60;
            struct in_addr ip;
            memcpy(&ip, &pnSeed[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
	    }
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet (v4)
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.

    	// Testnet Genesis has a lower difficulty
        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;

        vAlertPubKey = ParseHex("0468770c9d451dd5d6d373ae6096d4ab0705c4ab66e55cc25c40788580039bd04b7672322b9bd26ce22a3ad95f490d7d188a905ce30246b2425eca8cc5102190d0");
        nDefaultPort = 19265;
        nRPCPort = 19266;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 8);
        strDataDir = "testnet4";
	fStrictChainId = true;
	nAuxpowChainId = 0x005B;
	nEquihashN = 200;
	nEquihashK = 9;
	fMineBlocksOnDemand = false;

	const char* pszTimestamp = "Testing Testnet";
	CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 20 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04f88a76429dad346a10ecb5d36fcbf50bc2e009870e20c1a6df8db743e0b994afc1f91e079be8acc380b0ee7765519906e3d781519e9db48259f64160104939d8") << OP_CHECKSIG;
        genesis.vtx[0] = txNew;
	genesis.hashPrevBlock = 0;
	genesis.hashMerkleRoot = genesis.BuildMerkleTree();

        genesis.nTime = 1534873293;
        genesis.nBits = 0x1e0ffff0;
	genesis.nNonce = 181283;
	hashGenesisBlock = genesis.GetHash();
	//printf("hashGenesisBlock = %s\n",hashGenesisBlock.GetHex().c_str());
	//printf("powhash = %s\n",genesis.GetPoWHash().GetHex().c_str());
        assert(hashGenesisBlock == uint256("45ccef675b070c6eae865e1fcd3978253ec52a960af9abbb91bd1d935513e5be"));

        vFixedSeeds.clear();
	        for (unsigned int i = 0; i < ARRAYLEN(pnSeedTest); i++)
        {
            const int64_t nOneWeek = 7*24*60*60;
            struct in_addr ip;
            memcpy(&ip, &pnSeedTest[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
	}		
	
        vSeeds.clear();
	vSeeds.push_back(CDNSSeedData("bitmark.io", "us.bitmark.io"));
        vSeeds.push_back(CDNSSeedData("bitmark.cc", "seed.bitmark.cc"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,130); // u
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1,258);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nSubsidyHalvingInterval = 300;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1405274400;
        genesis.nBits = 0x207fffff;
	genesis.nNonce = 713058;
	/*
	CBigNum bnTarget;
	bnTarget.SetCompact(genesis.nBits);
	uint256 target = bnTarget.getuint256();
	printf("have to beat %s\n",target.GetHex().c_str());
	unsigned int curNonce = 0;
	while (1) {
	  genesis.nNonce = curNonce;
	  uint256 hash = genesis.GetPoWHash();
	  if (hash<=target) break;
	  curNonce++;
	}
	printf("nonce is %d\n",curNonce);
	*/
        hashGenesisBlock = genesis.GetHash();
	genesis.nNonce = 3;
        nDefaultPort = 18444;
        strDataDir = "regtest";
	fStrictChainId = true;
	nAuxpowChainId = 0x005B;
	nEquihashN = 200;
	nEquihashK = 9;
	fMineBlocksOnDemand = true;
	//nWorkExponent = 30;
	//printf("regtest hashGenesisBlock = %s\n",hashGenesisBlock.GetHex().c_str());
	//printf("powhashgenesis = %s\n",genesis.GetPoWHash().GetHex().c_str());
        assert(hashGenesisBlock == uint256("0x168329a349fc93768bfb02e536bbe1e1847d77a65764564552122fa9268d8841"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
	  pCurrentParams = &mainParams;
	  break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
    fReopenDebugLog = true;
}

/*
void SelectDifficulty (int difficulty) {
  
}
*/

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
