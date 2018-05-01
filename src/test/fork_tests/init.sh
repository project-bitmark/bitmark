#!/bin/bash

set -e

set +e
bitmark-cli stop
set -e

datadir="$(pwd)/.bitmark"
rm -rf "$datadir"
mkdir -p "$datadir"
cp bitmark.conf "$datadir"

set +e
for i in {1..10}
do
    sleep 1
    if bitmarkd -datadir="$datadir" -daemon
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
    if bitmark-cli -datadir="$datadir" getinfo
    then
	break
    fi
done
set -e

bitmark_cli="bitmark-cli -datadir=$datadir"

for i in {1..10}
do
    $bitmark_cli setgenerate true 1 1
    $bitmark_cli setgenerate true 1 2
    $bitmark_cli setgenerate true 1 3
    $bitmark_cli setgenerate true 1 4
    $bitmark_cli setgenerate true 1 5
done

for i in {1..10}
do
    $bitmark_cli setgenerate true 1 1
    $bitmark_cli setgenerate true 2 2
    $bitmark_cli setgenerate true 1 3
    $bitmark_cli setgenerate true 3 4
    $bitmark_cli setgenerate true 1 5
done

for i in {1..40}
do
    $bitmark_cli setgenerate true 3 1
    $bitmark_cli setgenerate true 3 2
    $bitmark_cli setgenerate true 3 3
    $bitmark_cli setgenerate true 3 4
    $bitmark_cli setgenerate true 3 5
done

# 730 blocks here

$bitmark_cli setgenerate true 1 1
$bitmark_cli setgenerate true 3 5
$bitmark_cli setgenerate true 1 1
$bitmark_cli setgenerate true 2 5

for i in {1..48}
do
    $bitmark_cli setgenerate true 1 1
    $bitmark_cli setgenerate true 3 5
done

# 929 blocks here

for i in {1..48}
do
    $bitmark_cli setgenerate true 3 1
    $bitmark_cli setgenerate true 3 5
done

#1217 blocks

$bitmark_cli move "" "a1" 50
$bitmark_cli sendfrom a1 ugDcD5iH4uTQFWBJxDiYJec75ijkYsn8w1 15
$bitmark_cli setgenerate true 1 1
$bitmark_cli setgenerate true 1 2

$bitmark_cli move "" "a2" 50
$bitmark_cli sendfrom a2 uXDLegVzqHRrPRT5TFsxR5o8x7Tcbsz1zS 16.42
$bitmark_cli setgenerate true 1 5

sleep 1

echo "TESTS INITIALIZED"
