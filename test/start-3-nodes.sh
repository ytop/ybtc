#!/bin/bash
#set -x
cd $PWD

echo "Create log path..."
rm -rf /tmp/ybtc
mkdir -p /tmp/ybtc/1
mkdir -p /tmp/ybtc/2
mkdir -p /tmp/ybtc/3

echo "Start node 1..."
${PWD}/../src/ybd  -daemon -datadir=/tmp/ybtc/1 -testnet  -server -listen -port=8871 -maxtipage=122223333  -txindex=1 -rest  -rpcuser=ybtc -rpcpassword=ybtc -rpcport=8881   -dnsseed=0 -dns=0  -connect=127.0.0.1:8872  -connect=127.0.0.1:8872  -addnode=127.0.0.1:8873  -addnode=127.0.0.1:8873 -gen -debug -rpcallowip=0.0.0.0/0

echo "sleep 40 seconds for genesis phase..."
sleep 40

echo "Start node 2..."
${PWD}/../src/ybd  -daemon -datadir=/tmp/ybtc/2 -testnet  -server -listen -port=8872 -maxtipage=122223333  -txindex=1 -rest  -rpcuser=ybtc -rpcpassword=ybtc   -rpcport=8882 -dnsseed=0 -dns=0  -connect=127.0.0.1:8871  -connect=127.0.0.1:8871  -addnode=127.0.0.1:8873  -addnode=127.0.0.1:8873 -gen -debug -rpcallowip=0.0.0.0/0
sleep 5 

echo "Start node 3..."
${PWD}/../src/ybd  -daemon -datadir=/tmp/ybtc/3  -testnet -server -listen -port=8873 -maxtipage=122223333  -txindex=1 -rest  -rpcuser=ybtc -rpcpassword=ybtc   -rpcport=8883 -dnsseed=0 -dns=0  -connect=127.0.0.1:8872  -connect=127.0.0.1:8872  -addnode=127.0.0.1:8871  -addnode=127.0.0.1:8871 -gen -debug -rpcallowip=0.0.0.0/0
sleep 5 

echo "All 3 nodes started"
