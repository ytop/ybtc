// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Ybtc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_SCFACE_HPP
#define FABCOIN_SCFACE_HPP

#include <txdb.h>
#include "validation.h"
#include "chainparams.h"
#include "script/standard.h"
#include "sccommon.h"
#include "sctransaction.h"


enum SmartContractFlags {
    ISCREATE,
    ISMESSAGECALL
};

class SmartContract {
private:
   
    bool ParseStack(std::vector<std::vector<unsigned char>>& stack);
   

public:
    SmartContract()  {};

    bool TxContractExec(const CTransaction& tx, sc::u256& refundGasAmount, std::vector<CTxOut>& vRefundGasFee, std::vector<unsigned char>& output);

    bool GetBlockContract(const CBlock& block, std::vector<CTxOut>& vRefundGasFee);

public:
    SmartContractFlags flag;
    sc::h160 contractAddress;
    sc::h160 callerAddress;
    std::vector<unsigned char> fabCallerAddress;
    std::vector<unsigned char> datahex;
    sc::u256 gasPrice;
    sc::u256 gasLimit;
    sc::u256 value;
   
};
#endif // FABCOIN_SCFACE_HPP

