#!/bin/bash

set -e

datadir="$(pwd)/.bitmark"

set +e
bitmark-cli -datadir="$datadir" stop
set -e

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

bitmarkcli="bitmark-cli -datadir=$datadir"

# activate fork
# Note: blocks 0 to 149 have subsidy 20, while the rest have subsidy 15
for i in {1..199}
do
    $bitmarkcli setgenerate true 1 0
done

# pre fork money supply: 149*20+50*15=3730
# so base ms for each algo is 466.25

# make 1 the algo for the first block using fork rules
$bitmarkcli setgenerate true 1 1

# height 200

for i in {1..10}
do
    $bitmarkcli setgenerate true 1 0
    $bitmarkcli setgenerate true 1 1
    $bitmarkcli setgenerate true 1 2
    $bitmarkcli setgenerate true 1 3
    $bitmarkcli setgenerate true 1 4
    $bitmarkcli setgenerate true 1 5
    $bitmarkcli setgenerate true 1 6
    $bitmarkcli setgenerate true 1 7
done

# height 280

for i in {1..10}
do
    $bitmarkcli setgenerate true 1 0
    $bitmarkcli setgenerate true 1 1
    $bitmarkcli setgenerate true 2 2
    $bitmarkcli setgenerate true 1 3
    $bitmarkcli setgenerate true 3 4
    $bitmarkcli setgenerate true 1 5
    $bitmarkcli setgenerate true 2 6
    $bitmarkcli setgenerate true 1 7
done

# height 400

for i in {1..20}
do
    $bitmarkcli setgenerate true 3 0
    $bitmarkcli setgenerate true 3 1
    $bitmarkcli setgenerate true 3 2
    $bitmarkcli setgenerate true 3 3
    $bitmarkcli setgenerate true 3 4
    $bitmarkcli setgenerate true 3 5
    $bitmarkcli setgenerate true 3 6
    $bitmarkcli setgenerate true 3 7
done

# height 880
# Post fork blocks for algos
# 0: 80
# 1: 81
# 2: 90
# 3: 80
# 4: 100
# 5: 80
# 6: 90
# 7: 80

$bitmarkcli setgenerate true 1 1
$bitmarkcli setgenerate true 3 5
$bitmarkcli setgenerate true 1 1
$bitmarkcli setgenerate true 7 5

# now 5 is at ssf block 89
# height 892

for i in {1..30}
do
    $bitmarkcli setgenerate true 1 1
    $bitmarkcli setgenerate true 3 5
done

curpeakhr=$($bitmarkcli chaindynamics | grep 'peak hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}')
curhr=$($bitmarkcli chaindynamics | grep 'current hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}') #p1
reward=$($bitmarkcli getblockreward 1 | grep 'block reward' | awk '{print $4}' | awk -F ',' '{print $1}')
echo "p1 curhr = $curhr curpeakhr = $curpeakhr reward = $reward"

# height 1012

for i in {1..30}
do
    $bitmarkcli setgenerate true 3 1
    $bitmarkcli setgenerate true 3 5
done

# height 1192

$bitmarkcli setgenerate true 1 5

# 3/2 more time to generate algo 5

# height 1193

$bitmarkcli move "" "a1" 50
$bitmarkcli sendfrom a1 ugDcD5iH4uTQFWBJxDiYJec75ijkYsn8w1 10
$bitmarkcli setgenerate true 1 1
$bitmarkcli setgenerate true 1 2

$bitmarkcli move "" "a2" 50
$bitmarkcli sendfrom a2 uXDLegVzqHRrPRT5TFsxR5o8x7Tcbsz1zS 16.42
$bitmarkcli setgenerate true 1 5

sleep 1

echo "TESTS INITIALIZED"
