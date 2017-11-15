#!/bin/bash

set -e

export SCRIPT_DIR=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")

. $SCRIPT_DIR/env.sh

mkdir -p $repo/scripts/gnunet1
mkdir -p $repo/scripts/gnunet2

trap "pkill 'gnunet-*' -9 || true" INT EXIT

gnunet-arm -s -c $cfg1 &
gnunet-arm -s -c $cfg2 &

sleep 1

echo "* Peer info on peer1 $(gnunet_id_of $cfg1)"
echo "* Peer info on peer2 $(gnunet_id_of $cfg2)"

# This is not necessary, but it makes developing easier/faster.
echo "Interconnecting the two"
gnunet-peerinfo -c $cfg2 -p `gnunet-peerinfo -c $cfg1 -g`

read
