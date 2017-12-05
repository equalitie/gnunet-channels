#!/bin/bash

# This script isn't meant to run manually, instead
# it is executed by running 'make test'.

set -e

CMAKE_SOURCE_DIR=$1
CMAKE_BINARY_DIR=$2

export LD_LIBRARY_PATH=$CMAKE_BINARY_DIR/gnunet/lib:$LD_LIBRARY_PATH
export PATH=$CMAKE_BINARY_DIR/gnunet/bin:$PATH

export GNUNET_TEST_HOME=$CMAKE_BINARY_DIR

cp $CMAKE_SOURCE_DIR/scripts/peer{1,2}.conf $CMAKE_BINARY_DIR/

export cfg1=$CMAKE_BINARY_DIR/peer1.conf
export cfg2=$CMAKE_BINARY_DIR/peer2.conf

mkdir -p $CMAKE_BINARY_DIR/gnunet1
mkdir -p $CMAKE_BINARY_DIR/gnunet2

trap "pkill 'gnunet-*' -9 || true" INT EXIT

gnunet-arm -s -c $cfg1 &
gnunet-arm -s -c $cfg2 &

# This is not necessary, but it makes developing easier/faster.
gnunet-peerinfo -c $cfg2 -p `gnunet-peerinfo -c $cfg1 -g`

sleep 5

$CMAKE_BINARY_DIR/tests --log_level=test_suite
