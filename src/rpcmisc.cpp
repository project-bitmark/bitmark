// Copyright (c) 2010 Satoshi Nakamoto
// Original Code: Copyright (c) 2009-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "init.h"
#include "main.h"
#include "alert.h"
#include "net.h"
#include "netbase.h"
#include "rpcserver.h"
#include "util.h"
#include "miner.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#include "walletdb.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total bitmark balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
            "  \"pow_algo_id\": n            (numeric) The active mining algorithm id\n"
            "  \"pow_algo\": \"name\"        (string) The active mining algorithm name\n"
//            "  \"difficulty <algo>\": xxxxxx,       (numeric) the current difficulty for the algo <ALGO>\n"
            "  \"difficulty_scrypt\": xxxxxx,   (numeric) the current scrypt difficulty\n"
            "  \"difficulty_sha256d\": xxxxxx,  (numeric) the current sha256d difficulty\n"
            "  \"difficulty_yescrypt\": xxxxxx, (numeric) the current yescrypt difficulty\n"
            "  \"difficulty_argon2d\": xxxxxx,    (numeric) the current argon2d difficulty\n"
            "  \"difficulty_x17\": xxxxxx,    (numeric) the current x17 difficulty\n"
            "  \"difficulty_lyra2rev2\": xxxxxx,    (numeric) the current lyra2rev2 difficulty\n"
            "  \"difficulty_equihash\": xxxxxx,  (numeric) the current equihash difficulty\n"
            "  \"difficulty_cryptonight\": xxxxxx,  (numeric) the current cryptonight difficulty\n"
            "  \"moneysupply\": xxxxxx,      (numeric) the total amount of coins distributed\n"
            "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in btm/kb\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for non-free transactions in btm/kb\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    if (!confAlgoIsSet) {
      miningAlgo = GetArg("-miningalgo", miningAlgo);
      confAlgoIsSet = true;
    }

    Object obj;
    obj.push_back(Pair("version",       (int)CLIENT_VERSION));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
        obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    }
#endif
    obj.push_back(Pair("blocks",        (int)chainActive.Height()));
    obj.push_back(Pair("timeoffset",    GetTimeOffset()));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("pow_algo_id", miningAlgo));
    obj.push_back(Pair("pow_algo",GetAlgoName(miningAlgo)));
    obj.push_back(Pair("difficulty", (double)GetDifficulty(NULL,miningAlgo,true,true)));
    obj.push_back(Pair("difficulty SCRYPT", (double)GetDifficulty(NULL,ALGO_SCRYPT,true,true)));
    obj.push_back(Pair("difficulty SHA256D",    (double)GetDifficulty(NULL,ALGO_SHA256D,true,true)));
    obj.push_back(Pair("difficulty YESCRYPT",    (double)GetDifficulty(NULL,ALGO_YESCRYPT,true,true)));
    obj.push_back(Pair("difficulty ARGON2",    (double)GetDifficulty(NULL,ALGO_ARGON2,true,true)));
    obj.push_back(Pair("difficulty X17",    (double)GetDifficulty(NULL,ALGO_X17,true,true)));
    obj.push_back(Pair("difficulty LYRA2REv2",    (double)GetDifficulty(NULL,ALGO_LYRA2REv2,true,true)));
    obj.push_back(Pair("difficulty EQUIHASH",    (double)GetDifficulty(NULL,ALGO_EQUIHASH,true,true)));
    obj.push_back(Pair("difficulty CRYPTONIGHT",    (double)GetDifficulty(NULL,ALGO_CRYPTONIGHT,true,true)));
    obj.push_back(Pair("moneysupply",    (double)GetMoneySupply(NULL,-1)));
    obj.push_back(Pair("testnet",       TestNet()));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(Pair("keypoololdest", pwalletMain->GetOldestKeyPoolTime()));
        obj.push_back(Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    }
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
#endif
    obj.push_back(Pair("relayfee",      ValueFromAmount(CTransaction::nMinRelayTxFee)));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<Object>
{
private:
    isminetype mine;

public:
    DescribeAddressVisitor(isminetype mineIn) : mine(mineIn) {}

    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        obj.push_back(Pair("isscript", false));
        if (mine == ISMINE_SPENDABLE) {
            pwalletMain->GetPubKey(keyID, vchPubKey);
            obj.push_back(Pair("pubkey", HexStr(vchPubKey)));
            obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        }

        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        if (mine != ISMINE_NO) {
            CScript subscript;
            pwalletMain->GetCScript(scriptID, subscript);
            std::vector<CTxDestination> addresses;
            txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.push_back(Pair("script", GetTxnOutputType(whichType)));
            obj.push_back(Pair("hex", HexStr(subscript.begin(), subscript.end())));
            Array a;
            BOOST_FOREACH(const CTxDestination& addr, addresses)
                a.push_back(CBitmarkAddress(addr).ToString());
            obj.push_back(Pair("addresses", a));
            if (whichType == TX_MULTISIG)
                obj.push_back(Pair("sigsrequired", nRequired));
        }
        return obj;
    }
};
#endif

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress \"bitmarkaddress\"\n"
            "\nReturn information about the given bitmark address.\n"
            "\nArguments:\n"
            "1. \"bitmarkaddress\"     (string, required) The bitmark address to validate\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,         (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"bitmarkaddress\", (string) The bitmark address validated\n"
            "  \"ismine\" : true|false,          (boolean) If the address is yours or not\n"
            "  \"isscript\" : true|false,        (boolean) If the key is a script\n"
            "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the raw public key\n"
            "  \"iscompressed\" : true|false,    (boolean) If the address is compressed\n"
            "  \"account\" : \"account\"         (string) The account associated with the address, \"\" is the default account\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
            + HelpExampleRpc("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
        );

    CBitmarkAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
#ifdef ENABLE_WALLET
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : ISMINE_NO;
        ret.push_back(Pair("ismine", (mine & ISMINE_SPENDABLE) ? true : false));
        if (mine != ISMINE_NO) {
            ret.push_back(Pair("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true: false));
            Object detail = boost::apply_visitor(DescribeAddressVisitor(mine), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest].name));
#endif
    }
    return ret;
}

//
// Used by addmultisigaddress / createmultisig:
//
CScript _createmultisig_redeemScript(const Array& params)
{
    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();
#ifdef ENABLE_WALLET
        // Case 1: Bitmark address and we have full public key:
        CBitmarkAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else
#endif
        if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result;
    result.SetMultisig(nRequired, pubkeys);
    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
        throw runtime_error(strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE));

    return result;
}

Value createmultisig(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 2)
    {
        string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are bitmark addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) bitmark address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"")
        ;
        throw runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(params);
    CScriptID innerID = inner.GetID();
    CBitmarkAddress address(innerID);

    Object result;
    result.push_back(Pair("address", address.ToString()));
    result.push_back(Pair("redeemScript", HexStr(inner.begin(), inner.end())));

    return result;
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"bitmarkaddress\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"bitmarkaddress\"  (string, required) The bitmark address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"signature\", \"my message\"")
        );

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CBitmarkAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

extern CCriticalSection cs_mapAlerts;
extern map<uint256, CAlert> mapAlerts;

// TODO verify and address
// There is a known deadlock situation with ThreadMessageHandler
// ThreadMessageHandler: holds cs_vSend and acquiring cs_main in SendMessages()
// ThreadRPCServer: holds cs_main and acquiring cs_vSend in alert.RelayTo()/PushMessage()/BeginMessage()
Value sendalert(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 8)
    throw runtime_error(
            "sendalert <message> <privatekey> <minver> <maxver> <priority> <id> [cancelupto]\n"
            "<message> is the alert text message\n"
            "<privatekey> is hex string of alert master private key\n"
            "<minver> is the minimum applicable internal client version\n"
            "<maxver> is the maximum applicable internal client version\n"
            "<priority> is integer priority number\n"
            "<id> is the alert id\n"
            "<relay> when should this message be relayed until\n"
            "<expiration> when does this alert expire\n"
            "[cancelupto] cancels all alert id's up to this number\n"
            "Returns true or false.");

    CAlert alert;
    CKey key;

    alert.strStatusBar = params[0].get_str();
    alert.nMinVer = params[2].get_int();
    alert.nMaxVer = params[3].get_int();
    alert.nPriority = params[4].get_int();
    alert.nID = params[5].get_int();
    if (params.size() > 8)
        alert.nCancel = params[8].get_int();
    alert.nVersion = PROTOCOL_VERSION;
    alert.nRelayUntil = GetAdjustedTime() + params[6].get_int();
    alert.nExpiration = GetAdjustedTime() + params[7].get_int();

    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << (CUnsignedAlert)alert;
    alert.vchMsg = vector<unsigned char>(sMsg.begin(), sMsg.end());

    vector<unsigned char> vchPrivKey = ParseHex(params[1].get_str());
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end()), false); // if key is not correct openssl may crash
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
        throw runtime_error(
            "Unable to sign alert, check private key?\n");
    if(!alert.ProcessAlert())
        throw runtime_error(
            "Failed to process alert.\n");
    // Relay alert
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            alert.RelayTo(pnode);
    }

    Object result;
    result.push_back(Pair("strStatusBar", alert.strStatusBar));
    result.push_back(Pair("nVersion", alert.nVersion));
    result.push_back(Pair("nMinVer", alert.nMinVer));
    result.push_back(Pair("nMaxVer", alert.nMaxVer));
    result.push_back(Pair("nPriority", alert.nPriority));
    result.push_back(Pair("nID", alert.nID));
    if (alert.nCancel > 0)
        result.push_back(Pair("nCancel", alert.nCancel));
    return result;
}

// Q? <<< Where is getblockspacing() used ?
Value getblockspacing(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "getblockspacing (algo interval height )\n"
            "Returns an object containing blockspacing info.\n"
	    "\nArguments:\n"
	    "1. \"algo\"     (numeric, optional) The algo, 2 (scrypt) by default\n"
            "2. \"interval\"     (numeric, optional) The interval in number of blocks, 24 by default\n"
	    "3. \"height\"     (numeric, optional) The height for the endpoint of the interval, tip by default\n"	    
	    "\nResult:\n"
	    "{\n"
	    "  \"average block spacing\": xxxxx           (numeric)\n"
	    "}\n"
			    );

    int algo = -1;
    int interval = 24;
    CBlockIndex * blockindex = NULL;
    
    if (params.size()>0) {
      algo = params[0].get_int();
      if (params.size()>1) {
	interval = params[1].get_int();
	if (params.size()>2) {
	  int height = params[2].get_int();

	  blockindex = chainActive.Tip();
          // walk the blockindex back until it matches requested height.
	  while (blockindex && blockindex->nHeight > height) {
	    blockindex = blockindex->pprev;
	  }
	}
      }
    }
    
    Object obj;
    obj.push_back(Pair("average block spacing",    (double)GetAverageBlockSpacing(blockindex,algo,interval)));

    return obj;
}

Value getblockreward(const Array& params, bool fHelp) {
  if (fHelp)
    throw runtime_error(
			"getblockreward (algo height )\n"
			"Returns an object containing blockreward info.\n"
	    "\nArguments:\n"
	    "1. \"algo\"     (numeric, optional) The algo, 2 (scrypt) by default\n"
	    "2. \"height\"     (numeric, optional) The height to look at, tip by default\n"	    
	    "\nResult:\n"
	    "{\n"
	    " \"block reward\": xxxxx           (numeric)\n"
	    "}\n"
			);
  
  int algo = ALGO_SCRYPT;
  CBlockIndex * blockindex = NULL;
	   
  if (params.size()>0) {
    algo = params[0].get_int();
    if (params.size()>1) {
	int height = params[1].get_int();
	blockindex = chainActive.Tip();
	while (blockindex && blockindex->nHeight > height) {
	  blockindex = blockindex->pprev;
	}
    }
  }

  Object obj;
  obj.push_back(Pair("block reward",(double)GetBlockReward(blockindex,algo,false)));
  return obj;
}

Value getmoneysupply(const Array& params, bool fHelp) {
  if (fHelp)
    throw runtime_error(
			"getmoneysupply ( algo height )\n"
			"Returns an object containing moneysupply info.\n"
				    "\nArguments:\n"
	    "1. \"algo\"     (numeric, optional) The algo, 0 (overall) by default\n"
	    "2. \"height\"     (numeric, optional) The height to look at, tip by default\n"	    
	    "\nResult:\n"
	    "{\n"
	    " \"money supply\": xxxxx           (numeric)\n"
	    "}\n"
			);

  int algo = -1;
  CBlockIndex * blockindex = NULL;

  if (params.size()>0) {
    algo = params[0].get_int();
    if (params.size()>1) {
      int height = params[1].get_int();
      blockindex = chainActive.Tip();
      while (blockindex && blockindex->nHeight > height) {
	blockindex = blockindex->pprev;
      }
    }
  }

  Object obj;
  obj.push_back(Pair("money supply",(double)GetMoneySupply(blockindex,algo)));
  return obj;
}

Value getdifficulty (const Array& params, bool fHelp) {
  if (fHelp)
    throw runtime_error(
			"getdifficulty ( algo height )\n"
			"Returns an object containing difficulty info.\n"
				    "\nArguments:\n"
	    "1. \"algo\"     (numeric, optional) The algo, 2 (scrypt) by default\n"
	    "2. \"height\"     (numeric, optional) The height to look at, tip by default\n"	    
	    "\nResult:\n"
	    "{\n"
	    " \"difficulty\": xxxxx           (numeric)\n"
	    "}\n"
			);

  int algo = ALGO_SCRYPT;
  CBlockIndex * blockindex = NULL;

  if (params.size()>0) {
    algo = params[0].get_int();
    if (params.size()>1) {
      int height = params[1].get_int();
      blockindex = chainActive.Tip();
      while (blockindex && blockindex->nHeight > height) {
	blockindex = blockindex->pprev;
      }
    }
  }

  Object obj;
  obj.push_back(Pair("difficulty",(double)GetDifficulty(blockindex,algo)));
  return obj;
}

Value chaindynamics(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "chain dynamics (height)\n"
            "Returns an object containing various state info.\n"
            "}\n"
	    "\nResult:\n"
	    "{\n"
	    " \"sdifficulty <algo>\": xxxxx           (numeric),\n"
	    " \"difficulty <algo>\": xxxxx           (numeric),\n"
	    " \"peak hashrate <algo>\": xxxxx           (numeric),\n"
	    " \"current hashrate <algo>\": xxxxx           (numeric),\n"
	    " \"nblocks update SSF <algo>\": xxxxx           (numeric),\n"
	    " \"average block spacing <algo>\": xxxxx           (numeric)\n"
	    "}\n"
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    CBlockIndex * pindex = 0;
    if (params.size()>0) {
      int height = params[0].get_int();
      pindex = chainActive.Tip();
      while (pindex && pindex->nHeight > height) {
	pindex = pindex->pprev;
      }
    }    
    
    Object obj;
    obj.push_back(Pair("pow_algo_id", miningAlgo));
    obj.push_back(Pair("pow_algo",GetAlgoName(miningAlgo)));
//  difficulty is weighted in Bitmark to more meaningfully compare relative values of competing chains
//                                       boolean for weighted / unweighted -------v
    obj.push_back(Pair("difficulty",      (double)GetDifficulty(NULL,miningAlgo,true,true)));
//  sdifficulty: the "simple", unweighted difficulty
    obj.push_back(Pair("sdifficulty",       (double)GetDifficulty(NULL,miningAlgo,false,true)));
    obj.push_back(Pair("sdifficulty SCRYPT", (double)GetDifficulty(NULL,ALGO_SCRYPT,false,true)));
    obj.push_back(Pair("sdifficulty SHA256D",    (double)GetDifficulty(NULL,ALGO_SHA256D,false,true)));
    obj.push_back(Pair("sdifficulty YESCRYPT",    (double)GetDifficulty(NULL,ALGO_YESCRYPT,false,true)));
    obj.push_back(Pair("sdifficulty ARGON2",    (double)GetDifficulty(NULL,ALGO_ARGON2,false,true)));
    obj.push_back(Pair("sdifficulty X17",    (double)GetDifficulty(NULL,ALGO_X17,false,true)));
    obj.push_back(Pair("sdifficulty LYRA2REv2",    (double)GetDifficulty(NULL,ALGO_LYRA2REv2,false,true)));
    obj.push_back(Pair("sdifficulty EQUIHASH",    (double)GetDifficulty(NULL,ALGO_EQUIHASH,false,true)));
    obj.push_back(Pair("sdifficulty CRYPTONIGHT",    (double)GetDifficulty(NULL,ALGO_CRYPTONIGHT,false,true)));

    obj.push_back(Pair("difficulty SCRYPT",    (double)GetDifficulty(pindex,ALGO_SCRYPT)));
    obj.push_back(Pair("difficulty SHA256D",    (double)GetDifficulty(pindex,ALGO_SHA256D)));
    obj.push_back(Pair("difficulty YESCRYPT",    (double)GetDifficulty(pindex,ALGO_YESCRYPT)));
    obj.push_back(Pair("difficulty ARGON2",    (double)GetDifficulty(pindex,ALGO_ARGON2)));
    obj.push_back(Pair("difficulty X17",    (double)GetDifficulty(pindex,ALGO_X17)));
    obj.push_back(Pair("difficulty LYRA2REv2",    (double)GetDifficulty(pindex,ALGO_LYRA2REv2)));
    obj.push_back(Pair("difficulty EQUIHASH",    (double)GetDifficulty(pindex,ALGO_EQUIHASH)));
    obj.push_back(Pair("difficulty CRYPTONIGHT",    (double)GetDifficulty(pindex,ALGO_CRYPTONIGHT)));
    obj.push_back(Pair("peak hashrate SCRYPT",    (double)GetPeakHashrate(pindex,ALGO_SCRYPT)));
    obj.push_back(Pair("peak hashrate SHA256D",    (double)GetPeakHashrate(pindex,ALGO_SHA256D)));
    obj.push_back(Pair("peak hashrate YESCRYPT",    (double)GetPeakHashrate(pindex,ALGO_YESCRYPT)));
    obj.push_back(Pair("peak hashrate ARGON2",    (double)GetPeakHashrate(pindex,ALGO_ARGON2)));
    obj.push_back(Pair("peak hashrate X17",    (double)GetPeakHashrate(pindex,ALGO_X17)));
    obj.push_back(Pair("peak hashrate LYRA2REv2",    (double)GetPeakHashrate(pindex,ALGO_LYRA2REv2)));
    obj.push_back(Pair("peak hashrate EQUIHASH",    (double)GetPeakHashrate(pindex,ALGO_EQUIHASH)));
    obj.push_back(Pair("peak hashrate CRYPTONIGHT",    (double)GetPeakHashrate(pindex,ALGO_CRYPTONIGHT)));
    obj.push_back(Pair("current hashrate SCRYPT",    (double)GetCurrentHashrate(pindex,ALGO_SCRYPT)));    
    obj.push_back(Pair("current hashrate SHA256D",    (double)GetCurrentHashrate(pindex,ALGO_SHA256D)));
    obj.push_back(Pair("current hashrate YESCRYPT",    (double)GetCurrentHashrate(pindex,ALGO_YESCRYPT)));
    obj.push_back(Pair("current hashrate ARGON2",    (double)GetCurrentHashrate(pindex,ALGO_ARGON2)));
    obj.push_back(Pair("current hashrate X17",    (double)GetCurrentHashrate(pindex,ALGO_X17)));
    obj.push_back(Pair("current hashrate LYRA2REv2",    (double)GetCurrentHashrate(pindex,ALGO_LYRA2REv2)));
    obj.push_back(Pair("current hashrate EQUIHASH",    (double)GetCurrentHashrate(pindex,ALGO_EQUIHASH)));
    obj.push_back(Pair("current hashrate CRYPTONIGHT",    (double)GetCurrentHashrate(pindex,ALGO_CRYPTONIGHT)));    
    obj.push_back(Pair("nblocks update SSF SCRYPT",    (int)GetNBlocksUpdateSSF(pindex,ALGO_SCRYPT)));
    obj.push_back(Pair("nblocks update SSF SHA256D",    (int)GetNBlocksUpdateSSF(pindex,ALGO_SHA256D)));
    obj.push_back(Pair("nblocks update SSF YESCRYPT",    (int)GetNBlocksUpdateSSF(pindex,ALGO_YESCRYPT)));
    obj.push_back(Pair("nblocks update SSF ARGON2",    (int)GetNBlocksUpdateSSF(pindex,ALGO_ARGON2)));
    obj.push_back(Pair("nblocks update SSF X17",    (int)GetNBlocksUpdateSSF(pindex,ALGO_X17)));
    obj.push_back(Pair("nblocks update SSF LYRA2REv2",    (int)GetNBlocksUpdateSSF(pindex,ALGO_LYRA2REv2)));
    obj.push_back(Pair("nblocks update SSF EQUIHASH",    (int)GetNBlocksUpdateSSF(pindex,ALGO_EQUIHASH)));
    obj.push_back(Pair("nblocks update SSF CRYPTONIGHT",    (int)GetNBlocksUpdateSSF(pindex,ALGO_CRYPTONIGHT)));
    obj.push_back(Pair("average block spacing SCRYPT",    (double)GetAverageBlockSpacing(pindex,ALGO_SCRYPT)));    
    obj.push_back(Pair("average block spacing SHA256D",    (double)GetAverageBlockSpacing(pindex,ALGO_SHA256D)));
    obj.push_back(Pair("average block spacing YESCRYPT",    (double)GetAverageBlockSpacing(pindex,ALGO_YESCRYPT)));
    obj.push_back(Pair("average block spacing ARGON2",    (double)GetAverageBlockSpacing(pindex,ALGO_ARGON2)));
    obj.push_back(Pair("average block spacing X17",    (double)GetAverageBlockSpacing(pindex,ALGO_X17)));
    obj.push_back(Pair("average block spacing LYRA2REv2",    (double)GetAverageBlockSpacing(pindex,ALGO_LYRA2REv2)));
    obj.push_back(Pair("average block spacing EQUIHASH",    (double)GetAverageBlockSpacing(pindex,ALGO_EQUIHASH)));
    obj.push_back(Pair("average block spacing CRYPTONIGHT",    (double)GetAverageBlockSpacing(pindex,ALGO_CRYPTONIGHT)));    

    return obj;
}

Value coins(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "coins (start_height end_height)\n"
            "Returns information about unspent outpoints created within the given range of blocks.\n"
            "}\n"
	    "\nResult:\n"
	    "{\n"
	    " \"sum of unspent outputs\": xxxxx           (numeric),\n"
	    "}\n"
        );

    int start_height = 0;
    int end_height = 0;
    CBlockIndex * pindex = chainActive.Tip();

    if (params.size() > 0) {
      start_height = params[0].get_int();
      if (params.size() > 1) {
	end_height = params[1].get_int();
	if (end_height > pindex->nHeight)
	  end_height = pindex->nHeight;
	while (pindex && pindex->nHeight > end_height) {
	  pindex = pindex->pprev;
	}
      }
      else {
	end_height = pindex->nHeight;
      }
    }
    else {
      end_height = pindex->nHeight;
      start_height = pindex->nHeight-5000;
    }

    CCoinsViewCache view(*pcoinsTip, true);
    int64_t nSat = 0;
    for (int h = end_height; h >= start_height; h--) {
      CBlock block;
      if (!ReadBlockFromDisk(block, pindex))	
	throw runtime_error ("can't read block\n");
      for (unsigned int i = 0; i < block.vtx.size(); i++) {
	const CTransaction tx = block.vtx[i];
	//LogPrintf("get coins for tx %s\n",tx.GetCachedHash().GetHex().c_str());
	if (!view.HaveCoins(tx.GetCachedHash()))
	  continue;
	const CCoins coins = view.GetCoins(tx.GetCachedHash());
	for (unsigned int j=0; j<tx.vout.size(); j++) {
	  if(coins.IsAvailable(j))
	    nSat += tx.vout[j].nValue;
	}
      }
      pindex = pindex->pprev;
    }

    Object obj;
    obj.push_back(Pair("sum of unspent outputs", ((double)nSat)/100000000.));

    return obj;
}
