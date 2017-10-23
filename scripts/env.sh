#!/bin/bash

export SCRIPT_DIR=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")

repo=$SCRIPT_DIR/..
root=$repo/build/gnunet

export LD_LIBRARY_PATH=$root/lib:$LD_LIBRARY_PATH
export PATH=$root/bin:$PATH

export GNUNET_TEST_HOME=$repo/scripts

export cfg1=$repo/scripts/peer1.conf
export cfg2=$repo/scripts/peer2.conf
