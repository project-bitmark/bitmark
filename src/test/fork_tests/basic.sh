#!/bin/bash

set -e

source assert.sh

datadir="$(pwd)/.bitmark"
bitmarkcli="bitmark-cli -datadir=$datadir"

ms0=$($bitmarkcli getmoneysupply 0 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms1=$($bitmarkcli getmoneysupply 1 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms2=$($bitmarkcli getmoneysupply 2 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms3=$($bitmarkcli getmoneysupply 3 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms4=$($bitmarkcli getmoneysupply 4 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms5=$($bitmarkcli getmoneysupply 5 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms6=$($bitmarkcli getmoneysupply 6 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms7=$($bitmarkcli getmoneysupply 7 880 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')

ASSERT_EQUALS $ms0 1668.75000000
ASSERT_EQUALS $ms1 1683.75000000
ASSERT_EQUALS $ms2 1818.75000000
ASSERT_EQUALS $ms3 1668.75000000
ASSERT_EQUALS $ms4 1968.75000000
ASSERT_EQUALS $ms5 1668.75000000
ASSERT_EQUALS $ms6 1818.75000000
ASSERT_EQUALS $ms7 1668.75000000

exit

prevblockhash="$($bitmarkcli getblockhash 880)"
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"CRYPTONIGHT\"
prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 139
prevblockreward=$($bitmarkcli getblockreward 7 730 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 15.00000000
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')

prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"CRYPTONIGHT\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"CRYPTONIGHT\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"EQUIHASH\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"EQUIHASH\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"EQUIHASH\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockhash=$($bitmarkcli getblock $prevblockhash | grep previousblockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"X17\"

prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 15

exit

prevblockheight=$($bitmarkcli getblock $prevblockhash | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')

prevblockheight=$(($prevblockheight - 17))
prevblockhash="$($bitmarkcli getblockhash $prevblockheight)"

prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"YESCRYPT\"

prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0

prevblockheight=$(($prevblockheight - 13))
prevblockhash="$($bitmarkcli getblockhash $prevblockheight)"

prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"YESCRYPT\"

prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 143

prevblockhash="$($bitmarkcli getblockhash 929)"
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0
prevblockreward=$($bitmarkcli getblockreward 5 929 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 20.00000000

prevblockhash="$($bitmarkcli getblockhash 1217)"
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"LYRA2REv2\"
prevblockssfheight=$($bitmarkcli getblock $prevblockhash | grep 'SSF height' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockssfheight" 0
prevblockreward=$($bitmarkcli getblockreward 5 1217 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_LESSTHAN "$prevblockreward" 20.00000000
prevblockreward=$($bitmarkcli getblockreward 5 1216 | grep '"block reward' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockreward" 20.00000000

prevblockhash="$($bitmarkcli getblockhash 1218)"
prevblockalgo=$($bitmarkcli getblock $prevblockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$prevblockalgo" \"SHA256D\"

address=$($bitmarkcli listtransactions a1 1 | grep address | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$address" \"ugDcD5iH4uTQFWBJxDiYJec75ijkYsn8w1\"
amount=$($bitmarkcli listtransactions a1 1 | grep amount | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$amount" "-15.00000000"
txid=$($bitmarkcli listtransactions a1 1 | grep txid | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
blockhash=$($bitmarkcli listtransactions a1 1 | grep blockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
height=$($bitmarkcli getblock "$blockhash" | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$height" 1218
algo=$($bitmarkcli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"SHA256D\"
blockhash=$($bitmarkcli getblockhash 1219)
algo=$($bitmarkcli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"YESCRYPT\"
set +e
txidmatch=$($bitmarkcli getblock $blockhash | grep $txid)
set -e
ASSERT_EQUALS "$txidmatch" ""
ms1a=$($bitmarkcli getmoneysupply 1 1217 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms1b=$($bitmarkcli getmoneysupply 1 1218 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms1b'-'$ms1a | bc -l)
ASSERT_EQUALS "$res" "20.00000000"
ms2a=$($bitmarkcli getmoneysupply 2 1218 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms2b=$($bitmarkcli getmoneysupply 2 1219 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms2b'-'$ms2a | bc -l)
ASSERT_EQUALS "$res" "20.00000000"

address=$($bitmarkcli listtransactions a2 1 | grep address | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$address" \"uXDLegVzqHRrPRT5TFsxR5o8x7Tcbsz1zS\"
amount=$($bitmarkcli listtransactions a2 1 | grep amount | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$amount" "-16.42000000"
txid=$($bitmarkcli listtransactions a2 1 | grep txid | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
blockhash=$($bitmarkcli listtransactions a2 1 | grep blockhash | awk '{ print $3 }' | awk -F ',' '{print $1}' | awk -F '"' '{print $2}')
height=$($bitmarkcli getblock "$blockhash" | grep '"height' | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$height" 1220
algo=$($bitmarkcli getblock $blockhash | grep algo | awk '{ print $3 }' | awk -F ',' '{print $1}')
ASSERT_EQUALS "$algo" \"LYRA2REv2\"
ms5a=$($bitmarkcli getmoneysupply 5 1219 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
ms5b=$($bitmarkcli getmoneysupply 5 1220 | grep '"money supply' | awk '{ print $4 }' | awk -F ',' '{print $1}')
res=$(echo $ms5b'-'$ms5a | bc -l)
ASSERT_LESSTHAN "$res" "20.0"
ASSERT_GREATERTHAN "$res" "0.0"

echo "TESTS PASSED"
