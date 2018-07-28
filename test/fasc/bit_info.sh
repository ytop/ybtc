#!/bin/bash

bit_info() {
  local nodenum="$1"
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo`
  BLOCK_NUM=`echo $TR|jq '.blocks'`
  BALANCE_NUM=`echo $TR|jq '.balance'`
  echo "node is " $nodenum "   blocks=" $BLOCK_NUM "   balance=" $BALANCE_NUM
  echo
  return $BLOCK_NUM 
}

