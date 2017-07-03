#!/bin/bash

function ASSERT_EQUALS {
    if [[ "$1" != "$2" ]]
    then
	echo "ASSERT EQUALS FAILS: $1 != $2"
	return 1
    fi
}

set -e

set +e
bitmark-cli stop
set -e

datadir=".bitmark"
rm -rf "$datadir"
mkdir -p "$datadir"
cp bitmark.conf "$datadir"

set +e
for i in {1..10}
do
    sleep 1
    if bitmarkd -daemon -datadir="$datadir"
    then
	break
    elif [[ "$i" == "10" ]]
    then
	echo "Please shutdown your instance of bitmarkd before starting the test"
    fi
done
set -e

set +e
while true
do
    sleep 1
    if bitmark-cli getinfo
    then
	break
    fi
done
set -e

for i in {1..10}
do
    bitmark-cli setgenerate true 1 1
    bitmark-cli setgenerate true 1 2
    bitmark-cli setgenerate true 1 3
    bitmark-cli setgenerate true 1 4
    bitmark-cli setgenerate true 1 5
done

for i in {1..10}
do
    bitmark-cli setgenerate true 1 1
    bitmark-cli setgenerate true 2 2
    bitmark-cli setgenerate true 1 3
    bitmark-cli setgenerate true 3 4
    bitmark-cli setgenerate true 1 5
done

for i in {1..40}
do
    bitmark-cli setgenerate true 3 1
    bitmark-cli setgenerate true 3 2
    bitmark-cli setgenerate true 3 3
    bitmark-cli setgenerate true 3 4
    bitmark-cli setgenerate true 3 5
done

sleep 1

ms1="$(bitmark-cli getinfo | grep 'moneysupply SHA256D' | awk '{ print $4 }' | awk -F ',' '{print $1}')"
ms2="$(bitmark-cli getinfo | grep 'moneysupply SCRYPT' | awk '{ print $4 }' | awk -F ',' '{print $1}')"
ms3="$(bitmark-cli getinfo | grep 'moneysupply ARGON2' | awk '{ print $4 }' | awk -F ',' '{print $1}')"
ms4="$(bitmark-cli getinfo | grep 'moneysupply X17' | awk '{ print $4 }' | awk -F ',' '{print $1}')"
ms5="$(bitmark-cli getinfo | grep 'moneysupply LYRA2REv2' | awk '{ print $4 }' | awk -F ',' '{print $1}')"

ASSERT_EQUALS "$ms1" 2804.00000000
ASSERT_EQUALS "$ms2" 3004.00000000
ASSERT_EQUALS "$ms3" 2804.00000000
ASSERT_EQUALS "$ms4" 3204.00000000
ASSERT_EQUALS "$ms5" 2804.00000000
