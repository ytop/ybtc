#include "scface.h"
#include "scvm.h"


// =================== Smart Contract ===========================


// Parse script stack to SmartContract Header
bool SmartContract::ParseStack(std::vector<std::vector<unsigned char>>& stack)
{
    
    stack.pop_back(); // pop op code
   
    auto _contractAddress = stack.back();
    contractAddress = sc::h160(_contractAddress);
    stack.pop_back();

    datahex = stack.back();
    stack.pop_back();

    auto _gasPrice = CScriptNum::vch_to_uint64(stack.back()); 
    gasPrice = sc::u256(_gasPrice);
    stack.pop_back();

    auto _gasLimit = CScriptNum::vch_to_uint64(stack.back());
    gasLimit = sc::u256(_gasLimit);
    stack.pop_back();

    fabCallerAddress = stack.back();
    callerAddress = sc::h160(fabCallerAddress);
    stack.pop_back();

    if (flag == ISCREATE) {
        value = sc::u256();
    } else {
        auto _value = CScriptNum::vch_to_uint64(stack.back());
        value = sc::u256(_value);
        stack.pop_back();
    }
    return true;
}



bool SmartContract::TxContractExec(const CTransaction& tx, sc::u256& refundGasAmount, std::vector<CTxOut>& vRefundGasFee, std::vector<unsigned char>& output)
{
    for (const auto& vout : tx.vout) {

        if (vout.scriptPubKey.HasOpCreate() || vout.scriptPubKey.HasOpCall()) {
            std::vector<std::vector<unsigned char>> stack;
            EvalScript(stack, vout.scriptPubKey, SCRIPT_EXEC_BYTE_CODE, BaseSignatureChecker(), SIGVERSION_BASE, nullptr);
            sc::Transaction scTx;
            flag = vout.scriptPubKey.HasOpCreate() ? ISCREATE : ISMESSAGECALL;
            // parse stack
            if (!ParseStack(stack)) {
                continue;
            }

            bool _isCreation = (flag == ISCREATE);
            scTx = std::move(sc::Transaction(_isCreation, value, gasPrice, gasLimit, contractAddress, datahex));
            scTx.forceSender(callerAddress);
            sc::h256 oldHashStateRoot(pState->rootHash());
            try {
                auto res = pState->execute(scTx);
                refundGasAmount = (gasLimit - res.gasUsed) * gasPrice;
                if (refundGasAmount > 0) {
                    CScript script(CScript() << OP_DUP << OP_HASH160 << fabCallerAddress << OP_EQUALVERIFY << OP_CHECKSIG);
                    vRefundGasFee.emplace_back(CTxOut(CAmount(refundGasAmount), script));
                }
            } catch (...) {
                pState->setRoot(oldHashStateRoot);
            }
        }
    }

    return true;
};

bool SmartContract::GetBlockContract(const CBlock& block, std::vector<CTxOut>& vRefundGasFee)
{
    
    // loop through transaction list,check if there is contract transaction
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        CTransaction tx (*(block.vtx[i]));
        if (tx.HasCreateOrCall()) {
            sc::u256 gasUsed = sc::h256(); 
            std::vector<unsigned char> txOutput;
            sc::h256 oldHashStateRoot(pState->rootHash());
            if (!TxContractExec(tx, gasUsed, vRefundGasFee, txOutput)) {
                pState->setRoot(oldHashStateRoot);
            };
        }
    }
    pState->commit(sc::State::CommitBehaviour::RemoveEmptyAccounts);
    return true;
};

