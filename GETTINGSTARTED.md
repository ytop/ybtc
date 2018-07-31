Getting Started
====================
Simulate 3-nodes test environment in one machine.

* Node 1 : Genesis node, miner
* Node 2 : Guest node, miner
* Node 3 : Guest node, miner


start 3 nodes
---------------------

```bash
cd ybtc/test
./start-3-nodes.sh
```



View debug.log
---------------------

```bash
tail -f /tmp/ybtc/1/debug.log # ctrl+c to quit
tail -f /tmp/ybtc/2/debug.log # ctrl+c to quit
tail -f /tmp/ybtc/3/debug.log # ctrl+c to quit
```

Check Casino Protocol activity
---------------------

```bash
grep CASINO /tmp/ybtc/1/debug.log
grep CASINO /tmp/ybtc/2/debug.log
grep CASINO /tmp/ybtc/3/debug.log
```



