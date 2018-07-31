Getting Started
====================

Ybtc is based on bitcoin branch 0.15. Original Bitcoin client/API calls list is suppoted in Ybtc.


Here is command map:

 command     | Bitcoin          | Ybtc
 ------------|------------------|----------------------
 Server      | bitcoind         | ybd
 Client      | bitcoin-cli      | ybc
 Tx Client   | bitcoin-tx       | ybt

Start 3 nodes
---------------------

Simulate 3-nodes test environment in one machine.

* Node 1 : Genesis node, miner
* Node 2 : Guest node, miner
* Node 3 : Guest node, miner


```bash
cd ybtc/test
./start-3-nodes.sh
```



View debug info
---------------------

```bash
tail -f /tmp/ybtc/1/testnet3/debug.log # ctrl+c to quit
tail -f /tmp/ybtc/2/testnet3/debug.log # ctrl+c to quit
tail -f /tmp/ybtc/3/testnet3/debug.log # ctrl+c to quit
```


Node 3 get casino address
---------------------
```bash
./ybc-3 getcasinoaddress
```
JSON-RPC response,

```bash
{
  "casinoaddress": "n1TTsqaWQBzWENLb32UkrWMaPVh3voYY9v"
}

```

Node 1 send node 3 $100 coins
---------------------

Receiver addres is from above response. This UTXO is for the gas fee of miner regestration in next step.

```bash
./ybc-1 sendtoaddress n1TTsqaWQBzWENLb32UkrWMaPVh3voYY9v 100

```

JSON-RPC response,
```bash
12decc7bc04a16d2f445486c37e01fc3fad57350245c83f83c31fd50e926a669

```

Node 3 sign in Casino Contract
---------------------

Send a message call "refill" to contract. Contract allocates free $20 for each register.

```bash
./ybc-3f n1TTsqaWQBzWENLb32UkrWMaPVh3voYY9v
```

JSON-RPC response,
```bash
{
  "txid": "ea7e09bb8e43e1598de809dabc8210cba3e9a1b7d639878eaebee724b2b42c32",
  "sender": "n1TTsqaWQBzWENLb32UkrWMaPVh3voYY9v",
  "hash160": "dab98bc9b1551b47e563b298d80fc8a32de29482"
}

```

Check contract status
---------------------
```bash
ybc-1 getcontractinfo miS9vC899fzcdYNNqzmHtS83pJkphxAvYZ

```

JSON-RPC response,
```bash
{
  "address": "miS9vC899fzcdYNNqzmHtS83pJkphxAvYZ",
  "store": {
    "storekey_0": 259,
    "storevalue_0": 19,
    "storekey_1": 2,
    "storevalue_1": 7345361395294285530,
    "storekey_2": 258,
    "storevalue_2": 20999995,
    "storekey_3": 1,
    "storevalue_3": 5,
    "storekey_4": 3,
    "storevalue_4": -2877861033691409278
  }
}

```

Check mining log in node 1
---------------------
```bash
grep CASINO /tmp/ybtc/1/testnet3/debug.log
```
```bash
CASINOMINER ---  phase 5 tip index 7
2018-07-31 17:07:30 CASINOMINER ---  moneyMe ******************** 
2018-07-31 17:07:32 CASINOMINER ---  player number next phase 2 
2018-07-31 17:07:32 CASINOMINER ---  set winner next phase 3938028100000000000000000000000000000000000000000000000000000000000000050000000000000000000000000000000000000000000000000000000000020001  
2018-07-31 17:07:32 CASINOMINER ---  casino in coinbase tx 
CASINOMINER ---  phase 6 tip index 0
2018-07-31 17:07:32 CASINO IsNextWinner input 43b70f130000000000000000000000000000000000000000000000000000000000000006 
2018-07-31 17:07:32 CASINO IsNextWinner return 0000000000000000000000000000000000000000000000000000000000000001 
2018-07-31 17:07:32 CASINO IsNextWinner 1 minerIndex 0 
2018-07-31 17:07:32 CASINOMINER --- inactive 
2018-07-31 17:07:56 CASINOMINER ---  nobody mine. I would dig more :( :( : ( :( :) :) :) :) 
CASINOMINER ---  phase 6 tip index 1
2018-07-31 17:07:58 CASINOMINER ---  moneyMe ******************** 
CASINOMINER ---  phase 6 tip index 2
2018-07-31 17:08:00 CASINOMINER --- inactive 
2018-07-31 17:08:08 CASINOMINER ---  nobody mine. I would dig more :( :( : ( :( :) :) :) :) 
CASINOMINER ---  phase 6 tip index 3
2018-07-31 17:08:10 CASINOMINER ---  moneyMe ******************** 
CASINOMINER ---  phase 6 tip index 4
2018-07-31 17:08:12 CASINOMINER --- inactive 
2018-07-31 17:08:20 CASINOMINER ---  nobody mine. I would dig more :( :( : ( :( :) :) :) :)
```

Check mining log in node 3
---------------------
```bash

grep CASINO /tmp/ybtc/3/testnet3/debug.log
```
```bash

CASINOMINER ---  phase 7 tip index 0
2018-07-31 17:08:38 CASINO IsNextWinner input 43b70f130000000000000000000000000000000000000000000000000000000000000007 
2018-07-31 17:08:38 CASINO IsNextWinner return 0000000000000000000000000000000000000000000000000000000000000002 
2018-07-31 17:08:38 CASINO IsNextWinner 2 minerIndex 1 
2018-07-31 17:08:38 CASINOMINER ---  moneyMe ******************** 
CASINOMINER ---  phase 7 tip index 1
2018-07-31 17:08:40 CASINOMINER --- inactive 
CASINOMINER ---  phase 7 tip index 2
2018-07-31 17:08:44 CASINOMINER ---  moneyMe ******************** 
CASINOMINER ---  phase 7 tip index 3
2018-07-31 17:08:46 CASINOMINER --- inactive 
2018-07-31 17:08:54 CASINOMINER ---  nobody mine. I would dig more :( :( : ( :( :) :) :) :) 
CASINOMINER ---  phase 7 tip index 4
2018-07-31 17:08:56 CASINOMINER ---  moneyMe ******************** 
CASINOMINER ---  phase 7 tip index 5
2018-07-31 17:08:58 CASINOMINER --- inactive 
CASINOMINER ---  phase 7 tip index 6
2018-07-31 17:09:02 CASINOMINER ---  moneyMe ********************
```


Check account balance in node 3
---------------------
```bash
ybc-3 listaccounts
```

JSON-RPC response,
```bash
{
  "": -36.00000000,
  "7UvVzb1m": 199.95565960
}
```




