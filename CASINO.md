Casino Consensus Protocol
====================
Miner from lucky guy.


Genesis Contract
---------------------

The Genesis Contract is created when chain kicks off. This super contract announces the rule for mining:

1. Miner chosen from contract only
2. Candidate miner has tokens
3. Minus 1 token for each candidate miner every phase
4. Winners are randomly selected by uniform distribution
5. The random seed in #4 is hash value from streamed winners' addresses in last phase

Phase and Winner
---------------------

Each phase has (P) blocks. There are (N) winners in one phase. P can be divided by N. 

N winners mine blocks with assigned sequence.

These dependencies are required:

 Net         | P        | N
 ------------|------------------|----------------------
 Test net      | 8          | 2
 Main net      | 16         | 16


Exception Handling
---------------------

The previous successful miners can pick up orphan block if assigned miner is absent. The more 
recently successful miner, the more possibility to be orphan picker.


Reward
---------------------

The miner has 4 rewards:

1. Coin base reward, 8 coin each block in 4 years. After that, each year 3% of total delivered coin.
2. Transaction fee
3. Contract gas fee
4. Casino token of last phase