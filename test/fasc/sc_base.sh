#!/bin/bash
source "./assert.sh"
source "./bit_env.sh"
source "./bit_daemon.sh"
source "./bit_mining.sh"
source "./bit_info.sh"
source "./bit_acct.sh"
source "./bit_bal.sh"
source "./bit_blockheight.sh"

export DAEMON_NUM=5

export BIT_PORT=15000
export RPC_PORT=14000

test_path=$(pwd)
export BIT_ROOT=${test_path}/../..

assert_eq "CODE" "TEST" "Test is quality!"

bit_env $DAEMON_NUM 
bit_daemon $DAEMON_NUM $BIT_ROOT $BIT_PORT $RPC_PORT

sleep 10 

echo
echo
echo "Initial mining  =================================================="
bit_info 1
BLOCK_NUM=$?

assert_eq "101" "$BLOCK_NUM" "failed: block # is not 101 !"

bit_mining $DAEMON_NUM $BIT_ROOT $RPC_PORT 10

bit_mining_one 1 $BIT_ROOT $RPC_PORT 100

echo
bit_info 1

echo
echo "Set account address =================================================="

port_rpc=$(($RPC_PORT + 1 ))
export ADDR_1=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 1 " ${ADDR_1}

port_rpc=$(($RPC_PORT + 2 ))
export ADDR_2=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 2 " ${ADDR_2}

port_rpc=$(($RPC_PORT + 3 ))
export ADDR_3=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 3 " ${ADDR_3}

port_rpc=$(($RPC_PORT + 4 ))
export ADDR_4=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 4 " ${ADDR_4}

port_rpc=$(($RPC_PORT + 5 ))
export ADDR_5=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 5 " ${ADDR_5}

port_rpc=$(($RPC_PORT + 4 ))
export SCADDR_4=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress "myContract"`
echo "contract address at node 4 " ${SCADDR_4}

echo
echo

echo
echo "get VM contract address =================================================="

port_rpc=$(($RPC_PORT + 1 ))
export VMADDR_1=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_1}`
export VMADDR_1=`echo ${VMADDR_1}|jq -r '.addressinvm'`
echo "VM address node 1 " ${VMADDR_1}

port_rpc=$(($RPC_PORT + 2 ))
export VMADDR_2=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_2}`
export VMADDR_2=`echo ${VMADDR_2}|jq -r '.addressinvm'`
echo "VM address node 2 " ${VMADDR_2}

port_rpc=$(($RPC_PORT + 3 ))
export VMADDR_3=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_3}`
export VMADDR_3=`echo ${VMADDR_3}|jq -r '.addressinvm'`
echo "VM address node 3 " ${VMADDR_3}

port_rpc=$(($RPC_PORT + 4 ))
export VMADDR_4=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_4}`
export VMADDR_4=`echo ${VMADDR_4}|jq -r '.addressinvm'`
echo "VM address node 4 " ${VMADDR_4}

port_rpc=$(($RPC_PORT + 5 ))
export VMADDR_5=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_5}`
export VMADDR_5=`echo ${VMADDR_5}|jq -r '.addressinvm'`
echo "VM address node 5 " ${VMADDR_5}

port_rpc=$(($RPC_PORT + 4 ))
export SCVMADDR_4=`${BIT_ROOT}/src/ybc -regtest -rpcuser=ybtcrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${SCADDR_4}`
export SCVMADDR_4=`echo ${SCVMADDR_4}|jq -r '.addressinvm'`
echo "contract VM address node 4 " ${SCVMADDR_4}
echo
echo


#### END: Set  account address #######################

