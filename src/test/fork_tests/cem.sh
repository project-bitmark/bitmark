#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

source assert.sh

datadir="$(pwd)/.bitmark"
bitmarkcli="bitmark-cli -datadir=$datadir"

nssf=$($bitmarkcli chaindynamics | grep 'nblocks update SSF SHA256D' | awk '{print $6}' | awk -F ',' '{print $1}')

ASSERT_EQUALS $nssf 67

nssf=$($bitmarkcli chaindynamics | grep 'nblocks update SSF EQUIHASH' | awk '{print $6}' | awk -F ',' '{print $1}')

ASSERT_EQUALS $nssf 1

for i in {1..66}
do
    $bitmarkcli setgenerate true 1 1
done

nssf=$($bitmarkcli chaindynamics | grep 'nblocks update SSF SHA256D' | awk '{print $6}' | awk -F ',' '{print $1}')

ASSERT_EQUALS $nssf 1

nssf=$($bitmarkcli chaindynamics | grep 'nblocks update SSF EQUIHASH' | awk '{print $6}' | awk -F ',' '{print $1}')

ASSERT_EQUALS $nssf 1

peakhr=$($bitmarkcli chaindynamics | grep 'peak hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}')
curhr=$($bitmarkcli chaindynamics | grep 'current hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}') #p2
reward=$($bitmarkcli getblockreward 1 | grep 'block reward' | awk '{print $4}' | awk -F ',' '{print $1}')
echo "p2 curhr = $curhr curpeakhr = $peakhr reward = $reward"

ASSERT_EQUALS $curhr $peakhr

for i in {1..90}
do
    $bitmarkcli setgenerate true 1 1
done

curpeakhr=$($bitmarkcli chaindynamics | grep 'peak hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}')
curhr=$($bitmarkcli chaindynamics | grep 'current hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}') #p3
reward=$($bitmarkcli getblockreward 1 | grep 'block reward' | awk '{print $4}' | awk -F ',' '{print $1}')
echo "p3 curhr = $curhr curpeakhr = $curpeakhr reward = $reward"

#ASSERT_EQUALS $curpeakhr $peakhr
#ASSERT_LESSTHAN $curhr $peakhr

# keep running it for over 365 periods to monitor the change in cur / peak hashrate
for i in {4..380}
do
    $bitmarkcli setgenerate true 90 1
    $bitmarkcli setgenerate true 50 5
    curhr=$($bitmarkcli chaindynamics | grep 'current hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}')
    curpeakhr=$($bitmarkcli chaindynamics | grep 'peak hashrate SHA256D' | awk '{print $5}' | awk -F ',' '{print $1}')
    reward=$($bitmarkcli getblockreward 1 | grep 'block reward' | awk '{print $4}' | awk -F ',' '{print $1}')
    echo "p$i curhr = $curhr curpeakhr = $curpeakhr reward = $reward"
done

# we now have 368 periods in total

echo "CEM TESTS PASSED"
