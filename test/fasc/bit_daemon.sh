#!/bin/bash
source "./bit_mining.sh"

bit_daemon() {
  local daemon_num="$1"
  local bit_root="$2"
  local bit_port=$3
  local rpc_port="$4"

  killall ybd
  sleep 3
  rm -fr ~/.ybtc
  for i in `seq 1 ${daemon_num}`;
    do
      portf=$(($bit_port + $i))
      portr=$(($rpc_port + $i))
      portn=$portf
     if [ "$i" != "$daemon_num" ]; then
       portn=$(($portf + 1))
     else
       portn=$(($bit_port + 1))
     fi

      addnode=" "
      for j in `seq 1 ${daemon_num}`;
      do
        portfn=$(($bit_port + $j))
        if [ "$portfn" != "$portf" ]; then
          addnode+=" -addnode=localhost:${portfn} "
        fi
      done

      ${bit_root}/src/ybd -server -listen -port=${portf} -rpcuser=ybtcrpc -rpcpassword=P0 -rpcport=${portr} -datadir=$HOME/regtest/${i}/ -connect=localhost:${portn} -regtest -pid=$HOME/regtest/${i}/.pid -daemon -debug  ${addnode}  -gen
     if [ "$i" != "1" ]; then
       sleep 1
     else
       sleep 10
       bit_mining_one 1 $BIT_ROOT $RPC_PORT 100
       sleep 3
     fi
    done    
}

