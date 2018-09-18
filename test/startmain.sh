#!/bin/bash
#set -x
cd $PWD

echo "Create log path..."
rm -rf /tmp/ybtc
mkdir -p /tmp/ybtc

echo "Start node 1..."
${PWD}/../src/ybd  -daemon -datadir=/tmp/ybtc -server -listen -port=8871 -maxtipage=122223333  -txindex=1 -rest  -rpcuser=ybtc -rpcpassword=ybtc -rpcport=8881   -dnsseed=0 -dns=0  -connect=127.0.0.1:8872  -addnode=127.0.0.1:8872 -gen -debug -rpcallowip=0.0.0.0/0
