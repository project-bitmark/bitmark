#!/bin/bash

set -e

source assert.sh

ms1=$(bitmark-cli getmoneysupply 1 730 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms2=$(bitmark-cli getmoneysupply 2 730 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms3=$(bitmark-cli getmoneysupply 3 730 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms4=$(bitmark-cli getmoneysupply 4 730 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms5=$(bitmark-cli getmoneysupply 5 730 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')

ASSERT_EQUALS "$ms1" 2804.00000000
ASSERT_EQUALS "$ms2" 3004.00000000
ASSERT_EQUALS "$ms3" 2804.00000000
ASSERT_EQUALS "$ms4" 3204.00000000
ASSERT_EQUALS "$ms5" 2804.00000000

prevblockhash="$(bitmark-cli getblockhash 730)"
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 139
prevblockreward=$(bitmark-cli getblockreward 5 730 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 20.00000000
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')

prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"X17\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"X17\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"X17\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"ARGON2\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"ARGON2\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"ARGON2\"
prevblockhash=$(bitmark-cli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"YESCRYPT\"

prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 5

prevblockheight=$(bitmark-cli getblock $prevblockhash | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')

prevblockheight=$(($prevblockheight - 17))
prevblockhash="$(bitmark-cli getblockhash $prevblockheight)"

prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"YESCRYPT\"

prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0

prevblockheight=$(($prevblockheight - 13))
prevblockhash="$(bitmark-cli getblockhash $prevblockheight)"

prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"YESCRYPT\"

prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 143

prevblockhash="$(bitmark-cli getblockhash 929)"
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0
prevblockreward=$(bitmark-cli getblockreward 5 929 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 20.00000000

prevblockhash="$(bitmark-cli getblockhash 1217)"
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockssfheight=$(bitmark-cli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0
prevblockreward=$(bitmark-cli getblockreward 5 1217 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_LESSTHAN "$prevblockreward" 20.00000000
prevblockreward=$(bitmark-cli getblockreward 5 1216 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 20.00000000

prevblockhash="$(bitmark-cli getblockhash 1218)"
prevblockalgo=$(bitmark-cli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"SHA256D\"

address=$(bitmark-cli listtransactions a1 1 | grep address | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$address" \"ugDcD5iH4uTQFWBJxDiYJec75ijkYsn8w1\"
amount=$(bitmark-cli listtransactions a1 1 | grep amount | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$amount" "-15.00000000"
txid=$(bitmark-cli listtransactions a1 1 | grep txid | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
blockhash=$(bitmark-cli listtransactions a1 1 | grep blockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
height=$(bitmark-cli getblock "$blockhash" | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$height" 1218
algo=$(bitmark-cli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"SHA256D\"
blockhash=$(bitmark-cli getblockhash 1219)
algo=$(bitmark-cli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"YESCRYPT\"
set +e
txidmatch=$(bitmark-cli getblock $blockhash | grep $txid)
set -e
ASSERT_EQUALS "$txidmatch" ""
ms1a=$(bitmark-cli getmoneysupply 1 1217 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms1b=$(bitmark-cli getmoneysupply 1 1218 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms1b'-'$ms1a | bc -l)
ASSERT_EQUALS "$res" "20.00000000"
ms2a=$(bitmark-cli getmoneysupply 2 1218 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms2b=$(bitmark-cli getmoneysupply 2 1219 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms2b'-'$ms2a | bc -l)
ASSERT_EQUALS "$res" "20.00000000"

address=$(bitmark-cli listtransactions a2 1 | grep address | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$address" \"uXDLegVzqHRrPRT5TFsxR5o8x7Tcbsz1zS\"
amount=$(bitmark-cli listtransactions a2 1 | grep amount | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$amount" "-16.42000000"
txid=$(bitmark-cli listtransactions a2 1 | grep txid | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
blockhash=$(bitmark-cli listtransactions a2 1 | grep blockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
height=$(bitmark-cli getblock "$blockhash" | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$height" 1220
algo=$(bitmark-cli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"LYRA2REv2\"
ms5a=$(bitmark-cli getmoneysupply 5 1219 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms5b=$(bitmark-cli getmoneysupply 5 1220 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms5b'-'$ms5a | bc -l)
ASSERT_LESSTHAN "$res" "20.0"
ASSERT_GREATERTHAN "$res" "0.0"

echo "TESTS PASSED"
