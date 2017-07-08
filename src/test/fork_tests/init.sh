#!/bin/bash

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

# 730 blocks here

bitmark-cli setgenerate true 1 1
bitmark-cli setgenerate true 3 5
bitmark-cli setgenerate true 1 1
bitmark-cli setgenerate true 2 5

for i in {1..48}
do
    bitmark-cli setgenerate true 1 1
    bitmark-cli setgenerate true 3 5
done

# 929 blocks here

for i in {1..48}
do
    bitmark-cli setgenerate true 3 1
    bitmark-cli setgenerate true 3 5
done

#1217 blocks

bitmark-cli move "" "a1" 50
bitmark-cli sendfrom a1 ugDcD5iH4uTQFWBJxDiYJec75ijkYsn8w1 15
bitmark-cli setgenerate true 1 1
bitmark-cli setgenerate true 1 2

bitmark-cli move "" "a2" 50
bitmark-cli sendfrom a2 uXDLegVzqHRrPRT5TFsxR5o8x7Tcbsz1zS 16.42
bitmark-cli setgenerate true 1 5

sleep 1

echo "TESTS INITIALIZED"
