#!/bin/bash

# This script isn't meant to run manually, instead
# it is executed by running 'make test'.

set -e

CMAKE_SOURCE_DIR=$1
CMAKE_BINARY_DIR=$2

if [ -z "$CMAKE_SOURCE_DIR" -o ! -d "$CMAKE_SOURCE_DIR" ]; then
    echo "This script is meant to be executed by cmake's test target"
    return 1
fi

if [ -z "$CMAKE_BINARY_DIR" -o ! -d "$CMAKE_BINARY_DIR" ]; then
    echo "This script is meant to be executed by cmake's test target"
    return 1
fi

export LD_LIBRARY_PATH=$CMAKE_BINARY_DIR/gnunet/lib:$LD_LIBRARY_PATH
export PATH=$CMAKE_BINARY_DIR/gnunet/bin:$PATH

mkdir -p $CMAKE_BINARY_DIR/test_dir
cd $CMAKE_BINARY_DIR/test_dir

export GNUNET_TEST_HOME=$CMAKE_BINARY_DIR/test_dir

cp $CMAKE_SOURCE_DIR/scripts/peer{1,2}.conf $CMAKE_BINARY_DIR/test_dir

export cfg1=$CMAKE_BINARY_DIR/test_dir/peer1.conf
export cfg2=$CMAKE_BINARY_DIR/test_dir/peer2.conf

mkdir -p $CMAKE_BINARY_DIR/test_dir/gnunet1
mkdir -p $CMAKE_BINARY_DIR/test_dir/gnunet2

trap "pkill 'gnunet-*' -9 || true" INT EXIT

gnunet-arm -s -c $cfg1 2>/dev/null &
gnunet-arm -s -c $cfg2 2>/dev/null &

# This is not necessary, but it makes developing easier/faster.
gnunet-peerinfo -c $cfg2 -p `gnunet-peerinfo -c $cfg1 -g`

sleep 5

$CMAKE_BINARY_DIR/tests --log_level=test_suite
