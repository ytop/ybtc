#!/bin/bash
export CASINOADDR2=`./ybc-2 getcasinoaddress|jq -r '.casinoaddress'`
echo $CASINOADDR2
./ybc-1 sendtoaddress ${CASINOADDR2}  100
./ybc-2f ${CASINOADDR2} 

export CASINOADDR3=`./ybc-3 getcasinoaddress|jq -r '.casinoaddress'`
echo $CASINOADDR3
./ybc-1 sendtoaddress ${CASINOADDR3}  100
./ybc-3f ${CASINOADDR3} 


