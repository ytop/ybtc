#!/bin/bash

bit_acct() {
  local nodenum="$1"
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} listaccounts`
  echo "node: " $nodenum " account: " $TR 
}

