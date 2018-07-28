#!/bin/bash

bit_blockheight() {
echo
echo "Block height now --- "
for i in `seq 1 ${1}`;
do
  local nodenum=$i
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo|jq '.blocks'`
  echo "node: " $nodenum " block height: " $TR

done
echo
}

