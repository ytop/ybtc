#!/bin/bash
source "./bit_acct.sh"

bit_bal() {
echo
echo "Balance now --- "
for i in `seq 1 ${1}`;
do
bit_acct $i
done
echo
}

