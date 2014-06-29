#!/bin/sh
# Copyright (c) 2013 The Bitcoin Core Developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Helper script for pull-tester.
#Param 1: path to bitmark srcroot
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [bitmark srcroot] build-test arguments..."
fi

killall -q bitmark-cli
killall -q bitmarkd

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
