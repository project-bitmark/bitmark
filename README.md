# Introduction

Project Bitmark is a multi faceted project which aims to provide:

1. A ** stable cryptographic currency network** which balances the requirements of all parties involved.
2. A **far reaching adoption initiative** under the guise of novel reputation+currency system called [Marking](https://github.com/project-bitmark/marking/wiki)

This repository contains the Bitmark cryptograpic currency software, and a wiki which provides all details pertaining to the software, it's configuration and the rationale of all design decisions.

# Bitmark

[![Join the chat at https://gitter.im/project-bitmark/bitmark](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/project-bitmark/bitmark?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Bitmark aims to be a relatively stable, user focused crypto currency, which refines and implements maturing innovations from the crypto currency sector.

### Overview
**Technically**: Light and stable with a modern codebase, [maturing features from the alternative currency sector](https://github.com/project-bitmark/bitmark/wiki#maturing-innovations) which *benefit* users added on a faster timeline than bitcoin. Think of it more as [standardization rather than innovation](https://github.com/project-bitmark/bitmark/wiki#relatively-stable).

**User focused**: Daily development effort and innovation goes in to making bitmark as [user friendly, and simple to integrate](https://github.com/project-bitmark/bitmark/wiki#user-focused) as possible.

**Earned Value**: Every aspect of Project Bitmark is focused on earned value. It's a project to make a viable every day currency, any value will be earned. 

**Longevity**: The Bitmark network has been operational since mid July, 2014. A well funded Bitmark Foundation is being created and funded by the community to support long term development. 

**Distribution**: A [configuration](https://github.com/project-bitmark/bitmark/wiki#block-chain-parameters) which aims to ensure [fair distribution](https://github.com/project-bitmark/bitmark/wiki/Currency#supply-and-distribution) whilst using [proven PoW](https://github.com/project-bitmark/bitmark/wiki#proof-of-work) which has had substantial investment in hardware. Extensive distribution of the currency is achieved through wide spread adoption under the banner of [Marking](https://github.com/project-bitmark/marking/wiki).

**Balance**: Every aspect of Bitmark has been designed in order to balance the interests of everybody involved or associated with the project. This includes [Investor Public Mining (IPM)](https://github.com/project-bitmark/bitmark/wiki/IPM-Pool) which balances Investors, Miners, and Developers in a way that is mutually beneficial and which critically enables those at the core to provide the best network experience to those who rely on it, the users.

* [Documentation and Plan](https://github.com/project-bitmark/bitmark/wiki)
* [Discussion and Updates](https://bitcointalk.org/index.php?topic=660544.0)
* [Block Explorer](http://explorer.bitmark.co)
* [Testnet Explorer](http://explorer.bitmark.co/testnet)

## Getting Bitmark

All Bitmark software releases are published through the github release process, you can download the [latest release](https://github.com/project-bitmark/bitmark/releases) from the releases tab above.

## Hard Fork #2 - (v0.9.8.0)
## CEM Increased Effectiveness & Authority - Equitable AuxPoW Blocks Subsidies - 

There are two major changes in Bitmark v0.9.8

1) 	
	A more effective Coin Emission Modulation policy. Instead of ruling over only one-half the full epoch reward, CEM v0.2 now has reduction authority over 80% of the epoch reward.

2) 	
	Block Rewards for Merge-Mined Blocks are now 20% of the CEM v0.2 modulated subsidy value. The rationale is that Merge-Mined blocks are much less expensive to mine.
  
Native rewards may range from 20% to 100% of the Epoch maximum (currently 15 MARKS) under CEM v0.2.  
This is scaled to the range 10% to 20% for merged mined blocks
For example if a native reward is 9 MARKS, which is in the middle of the 3 - 15 MARKS range, it will be  scaled for merge-mined blocks to 2.25 BTM, which is in the middle of range from 1.5 to 3BTM

## CEM - Coin Emission Modulation, version 0.2

The Coin Emission Modulation algorithm look at a past window of hashrate history, and chooses the hightest hashrate it finds as measure to compare the current hashrate against. When current hashrate is at or above this level, the epoch's full block reward subsidy will be given; but if the current hash rate is below, then the block reward gets  proportionately reduced, down to a certain minimum.  CEM tracks the hash rate for each PoW algo independently.

(Hard Fork #1, which introduced multiple proof-of-work algorithms (SCRYPT, SHA256D, YESCRYPT, ARGON2D, X17, LYRA2REv2, EQUIHASH, CRYPTONIGHT) under DGWv3 was adopted by the Bitmark community by supermajority consensus after block 450946. The first block where v0.9.7 rules are in effect is 450947.) 
Fork #1 has been very succesful.  The blockchain is flowing in a secure and steady way. Transactions are processed expediently, and CEM v0.1 has tempered coin emission with reduced subsidies when hashrate dips.

This fork, Hard Fork #2 will build on the success of CEM 0.1 by increasing the portion of the block reward over which the Coin Emission Modulation algorithm has reduction authority. This hard fork is to take effect on block 511,111. The Bitmark Foundation wishes that the blockchain retain a measure of independent and native mining. 
	We appreciate the extra hashrate and chain security brought by merge-miners and thank the Mining Pools which have added Bitmark to their mining operations. Because merge-mining costs are already established and mostly fixed when Bitmark  (or any other AuxPoW chain) is added through an AuxPoW-enabled algo to an existing mining operation, the extra work to obtain Bitmarks is relatively small.  AuxPoW block rewards should be equitable when compared with the work to mine blockas natively. 
We seek to find middle ground between the security value to the Bitmark chain and the marginal cost of production to merge-miners.
	Our goal is to foster native mining and to foster interest in the Bitmark project while still making it profitable and attractive for merge-miners of larger chains to contribute to Bitmark’s blockchain security. Any miner who wishes to receive the native CEM block subsidy may of course simply mine the chain natively. 


## Hard Fork #1 (v0.9.7)
## Eight Algortihm multiple Proof-of-Work Algorithms - 8mPoW 

The hard fork for multiple proof-of-work algorithms (SCRYPT, SHA256D, YESCRYPT, ARGON2D, X17, LYRA2REv2, EQUIHASH, CRYPTONIGHT) under DGWv3 was adopted by the Bitmark community by supermajority consensus after block 450946. The first block where v0.9.7 rules are in effect is 450947. 

Each algorithm has its difficulty adjusted independently, with a target spacing of 16 min (for a combined average of  2 min as per the Bitmark specification). 
The subsidy reduces at the same emission points as before, but each algorithm contributes only 1/8 of the number of emitted coins. The peak hash rate that determines the subsidy scaling factor is dynamic (depends on at most 1 year of hashing history for each algorithm) and the scaling factor remains constant throughout each 24 hour period (it is updated every 90 blocks per algo, which is approximately once a day ). 
The difficulty adjustment algorithm is Dark Gravity Wave v3, customized for multi algo usage. It has special difficulty retargeting rules, the Surge Protector and the Resurrector. Surge Protector activates if there is a string of 9 blocks in-a-row by the same algorithm. Resurrector activates if blocks from one algo are halted for over 160 minutes.

Download the current version from 'Master' branch, and put the following settings in your bitmark.conf

rpcuser=bitmarkrpc
rpcpassword=YoUrSecreT-PaSsWoRd
listen=1


##Note: libsodium cryptographic library is required by Bitmark v0.9.7 and above

Ubuntu 16 and higher may simply do:

  sudo apt-get install libsodium-dev

otherwise, you may compile this library from sources::

  git clone git://github.com/jedisct1/libsodium.git
  cd libsodium
  ./autogen.sh
  ./configure && make check
  sudo make install
  sudo ldconfig


## Mining
You can mine all algorithms using our fork of cpuminer-multi (https://github.com/piratelinux/cpuminer-multi default linux branch), with the following command:

`cpuminer -a <algo> -o http://localhost:19266 -u bitmarkrpc -p YoUrPaSsWoRd`

`<algo>` is `sha256d`, `scrypt`, `ar2`, `x17`, `lyra2REV2`, `equihash`, or `cryptonight`.

Note: The `miningAlgo` variable in src/rpcmining.cpp (choose one from `ALGO_SCRYPT` (0), `ALGO_SHA256D` (1), `ALGO_YESCRYPT` (2), `ALGO_ARGON2` (3), `ALGO_X17` (4), `ALGO_LYRA2REv2` (5), `ALGO_EQUIHASH` (6), `ALGO_CRYPTONIGHT` (7).
      
You can also control the mining algo via the rpcommand `bitmark-cli setminingalgo <algo number>`.

You can also use the bitmark-cli command to mine. The last parameter is the algorithm number as defined in core.h. For example, to mine argon2,

`bitmark-cli setgenerate true <ncores> 3`

For equihash and cryptonight, it is recommended to use the built in miner as the cpuminer is slow at the moment.

There are currently 3 types of tests implemented.

1) The testnet block explorer with details for each block, general stats, and charts. These tests are good for looking at how the difficulty adjusts with time.

2) The built in testing framework in src/test. After you compile Bitmark, you can run the tests using the executable `test_bitmark`. Some of the old tests have been disabled, but after disabling a few, all tests should pass now.

3) The newly built tests for the fork, in the directory src/test/fork_tests. To run the tests, you can execute the script `run_tests.sh` and you should see the message "TESTS PASSED" at the end. These ones use the constant min difficulty `regtest` network to perform efficient tests that need to quickly generate a large number of blocks.

## Merge Mining

Also part of the hard fork is merge mining, a way to increase the hashpower security of the chain by allowing the mining of the chain simultaneously with other chains. All 8 algorithms are supported.
