// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Ybtc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "base58.h"
#include "casino.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/tx_verify.h"
#include "consensus/validation.h"
#include "hash.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validation.h"
#include "validationinterface.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"

#include <algorithm>
#include <queue>
#include <utility>
#include <random>
#include <unordered_set>

//////////////////////////////////////////////////////////////////////////////
//
// YbtcMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest fee rate of a transaction combined with all
// its ancestors.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockWeight = 0;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options()
{
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
}

static BlockAssembler::Options DefaultOptions(const CChainParams& params)
{
    // Block resource limits
    // If neither -blockmaxsize or -blockmaxweight is given, limit to DEFAULT_BLOCK_MAX_*
    // If only one is given, only restrict the specified resource.
    // If both are given, restrict both.
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (gArgs.IsArgSet("-blockmintxfee")) {
        CAmount n = 0;
        ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n);
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions(params)) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}

void BlockAssembler::RebuildRefundTransaction(const std::vector<CTxOut>& vRefundGasFee)
{
    int refundtx = 0; //0 for coinbase in PoW

    CMutableTransaction contrTx(originalRewardTx);
    contrTx.vout[refundtx].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());

    if (vRefundGasFee.size() > 0) {
        int i = contrTx.vout.size();
        contrTx.vout.resize(contrTx.vout.size() + vRefundGasFee.size());
        for (auto& gasFee : vRefundGasFee) {
            contrTx.vout[refundtx].nValue -= gasFee.nValue;
            contrTx.vout[i] = gasFee;
            i++;
        }
    }

    pblock->vtx[refundtx] = MakeTransactionRef(std::move(contrTx));
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx)
{
    //int64_t nTimeStart = GetTimeMicros();

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if (!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1);       // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    pblock->nTime = GetAdjustedTime();
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST) ? nMedianTimePast : pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus()) && fMineWitnessTx;

    sc::h256 oldHashStateRoot(pState->rootHash());
    SmartContract smct;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;

    AddCasinoToCoinBaseTx(smct, coinbaseTx, nHeight);

    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));


    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;

    bool hasContract = false;
    std::vector<CTxOut> vRefundGasFee = std::vector<CTxOut>();
    addPackageTxs(smct, nPackagesSelected, nDescendantsUpdated, hasContract, vRefundGasFee);

    pblock->hashStateRoot = uint256(sc::h256Touint(sc::h256(pState->rootHash())));
    pState->setRoot(oldHashStateRoot);

    //int64_t nTime1 = GetTimeMicros();

    nLastBlockTx = nBlockTx;
    nLastBlockWeight = nBlockWeight;


    // Refund gas fee
    if (hasContract && vRefundGasFee.size() > 0)
        RebuildRefundTransaction(vRefundGasFee);

    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
    pblocktemplate->vTxFees[0] = -nFees;

    // jyan LogPrintf("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, nFees, nBlockSigOpsCost);

    // Fill in header
    pblock->hashPrevBlock = pindexPrev->GetBlockHash();
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits = 0;
    pblock->nHeight = nHeight;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    CValidationState state;
    if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    //int64_t nTime2 = GetTimeMicros();

    //jyan LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end();) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        } else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost)
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (const CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

// ================== Casino Contract ===========================

bool BlockAssembler::GenerateCasinoList(std::vector<int>& winner, uint32_t totalPlayer, unsigned int seed)
{
    auto winnerSize = winner.size();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, totalPlayer);
    std::unordered_set<int> randomset;

    gen.seed(seed);
    uint32_t i=0, k=0;
    while (i < winnerSize && k++ < winnerSize*100 ) {
        int randomX = dis(gen);
        if ( randomset.count(randomX)) continue;
        randomset.insert(randomX);
        winner[i++] = randomX;
    }
    return true;
}


bool BlockAssembler::AddCasinoToCoinBaseTx(SmartContract& smct, CMutableTransaction& coinbaseTx, int& nHeight)
{
    if (nHeight < 1) return true;
    bool hasCasino = false;
    CScript scriptPubKey;

    // Create casino contract

    if (nHeight == 1) {
        scriptPubKey = CScript() << minerAddress << CScriptNum(60000000) << CScriptNum(25) 
            << ParseHex(GENESIS_CONTRACT_CODE) << ParseHex(GENESIS_CONTRACT_ADDRESS_ETH) << OP_CREATE;
        hasCasino = true;
    }

    // Casino to set next phase winners
    if ( nHeight > 2 && ((nHeight - 1) % CHAIN_PHASE_SIZE) == CHAIN_PHASE_SIZE - 1) {

        auto currentPhase = (nHeight - 1) / CHAIN_PHASE_SIZE;

        // Get next phase player number
        std::vector<unsigned char> output;
        auto dataTotalPlayer = CASINO_GETTOTALPLAYER;
        CallContract(sc::h160(ParseHex(GENESIS_CONTRACT_ADDRESS_ETH)), ParseHex(dataTotalPlayer), &output);
        uint32_t totalPlayer = ConvertHexStringToUnsignedInt(output);
        LogPrint(BCLog::BENCH, "CASINOMINER ---  player number next phase %d \n", totalPlayer);
        if (totalPlayer == 0) totalPlayer = CHAIN_PHASE_PLAYER;

        // Get next phase seed 
        std::vector<unsigned char> output_seed;
        auto prevPhase = currentPhase > 0 ? currentPhase - 1 : 0;
        auto dataSeed = CASINO_GETWINNERSEED + str60zero + ConvertUnsignedIntToHexString(prevPhase);
        CallContract(sc::h160(ParseHex(GENESIS_CONTRACT_ADDRESS_ETH)), ParseHex(dataSeed), &output_seed);
        uint32_t winnerSeed = ConvertHexStringToUnsignedInt(output_seed);
        LogPrint(BCLog::BENCH, "CASINOMINER ---  seed next phase %d \n", winnerSeed);


        // Lottery game
        std::vector<int> winner(CHAIN_PHASE_PLAYER, 0);
        GenerateCasinoList(winner, totalPlayer, winnerSeed);

        std::string winstr = str64zero.substr(0, 64 - (4 * CHAIN_PHASE_PLAYER));
        for (uint32_t i = 0; i < CHAIN_PHASE_PLAYER; i++) {
            winstr += ConvertUnsignedIntToHexString(winner[i]);
        }

        // Set next phase winners
        auto datahex = CASINO_SETNEXTWINNERS + str60zero + ConvertUnsignedIntToHexString(currentPhase) + winstr;

        LogPrintf("CASINOMINER ---  set winner next phase %s  \n\n", datahex);
        
        scriptPubKey = CScript() << CScriptNum(0) << minerAddress << CScriptNum(60000000) 
            << CScriptNum(25) << ParseHex(datahex) << ParseHex(GENESIS_CONTRACT_ADDRESS_ETH) << OP_CALL;
        hasCasino = true;
    }

    // refresh pState
    if (hasCasino) {
        LogPrintf("CASINOMINER ---  casino in coinbase tx \n");
        CTxOut txout(0, scriptPubKey);
        coinbaseTx.vout.push_back(txout);

        sc::u256 refundGasAmount = 0;
        std::vector<unsigned char> txOutput;
        std::vector<CTxOut> vRefundGasFee;
        sc::h256 oldHashStateRoot(pState->rootHash());
        if (!smct.TxContractExec(coinbaseTx, refundGasAmount, vRefundGasFee, txOutput)) {
            pState->setRoot(oldHashStateRoot);
            LogPrintf("CASINOMINER --- Casino contract quit abnormally ????????????????? ");
            return false;
        }
    }

    return true;
}

// ================== Smart Contract ===========================

bool BlockAssembler::AttemptToAddContractToBlock(SmartContract& smct, CTxMemPool::txiter iter, std::vector<CTxOut>& vRefundGasFee)
{
    CMutableTransaction tx(iter->GetTx());
    LogPrintf("================================ txid %s\n", iter->GetTx().GetHash().ToString());

    sc::u256 refundGasAmount = 0;
    std::vector<unsigned char> txOutput;
    sc::h256 oldHashStateRoot(pState->rootHash());
    if (!smct.TxContractExec(tx, refundGasAmount, vRefundGasFee, txOutput)) {
        pState->setRoot(oldHashStateRoot);
        LogPrintf("Contract quit abnormally. ");
        return false;
    }

    CTransactionRef txref = std::make_shared<CTransaction>(tx);
    pblock->vtx.emplace_back(txref);
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    this->nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    this->nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
            CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
            iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
    indexed_modified_transaction_set& mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (const CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set& mapModifiedTx, CTxMemPool::setEntries& failedTx)
{
    assert(it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(SmartContract& smct, int& nPackagesSelected, int& nDescendantsUpdated, bool& hasContract, std::vector<CTxOut>& vRefundGasFee)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty()) {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
            SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                                                                     nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, iter, sortedEntries);

        for (size_t i = 0; i < sortedEntries.size(); ++i) {
            const CTransaction& tx = sortedEntries[i]->GetTx();
            if (tx.HasCreateOrCall()) {
                if (!AttemptToAddContractToBlock(smct, sortedEntries[i], vRefundGasFee)) {
                    continue;
                }
                hasContract = true;
            } else {
                AddToBlock(sortedEntries[i]);
            }

            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock) {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

#ifdef ENABLE_WALLET
//
#include "net.h"

static bool ProcessBlockFound(const CBlock* pblock, const CChainParams& chainparams)
{
    // jyan LogPrintf("%s\n", pblock->ToString());
    // jyan LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0]->vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("YbtcMiner: generated block is stale");
    }

    // Inform about the new block
    GetMainSignals().BlockFound(pblock->GetHash());

    // Process this block the same as if we had received it from another node
    bool fNewBlock = false;
    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    if (!ProcessNewBlock(chainparams, shared_pblock, true, &fNewBlock))
        return error("YbtcMiner: ProcessNewBlock, block not accepted");

    return true;
}

// Casino Miner

static uint32_t minerIndex = CHAIN_PHASE_PLAYER;

static void getMinerAddress()
{
    char myAccount[9];
    GenerateRandom8Char(myAccount);

    for (CWalletRef pwallet : ::vpwallets) {
        LogPrintf("wallet is %s\n", pwallet->GetName());
    }

    CWallet* const pwallet = ::vpwallets[0];
    LOCK2(cs_main, pwallet->cs_wallet);
    std::set<CTxDestination> setAddress = pwallet->GetAccountAddresses(std::string(myAccount));
    CPubKey pubKey;
    auto strAccount = std::string(myAccount);
    if (!pwallet->GetAccountPubkey(pubKey, strAccount)) {
        //TODO-J
    }
    auto address = CYbtcAddress(pubKey.GetID());
    minerAddress = ToByteVector(boost::get<CKeyID>(address.Get()));
}

static uint32_t IsActiveMiner(int currentPhase)
{
    // Go to contrac, check permission
    std::vector<unsigned char> output;
    auto data = CASINO_ISWINNERME + str60zero + ConvertUnsignedIntToHexString(currentPhase);
    LogPrintf("CASINO IsNextWinner input %s \n", data);
    CallContract(sc::h160(ParseHex(GENESIS_CONTRACT_ADDRESS_ETH)), ParseHex(data), &output);
    LogPrintf("CASINO IsNextWinner return %s \n", HexStr(output));
    return ConvertHexStringToUnsignedInt(output);
}

static bool IsNextMiner(int currentPhase, int currentIndex, int* checkedWinPhase)
{
    //return true;
    auto winner = -1;
    if (currentPhase >= 0 && (*checkedWinPhase) < currentPhase) {
        winner = IsActiveMiner(currentPhase);
        *checkedWinPhase = currentPhase;
        if (winner > 0) {
            minerIndex = winner - 1;
        } else {
            minerIndex = CHAIN_PHASE_PLAYER;
        }
        LogPrintf("CASINO IsNextWinner %d minerIndex %d \n", winner, minerIndex);
    }

    //Compare to minerIndex , if I am the next one
    return ((currentIndex + 1) % CHAIN_PHASE_PLAYER) == minerIndex;
}

int whenJumpAbnormal(int currentPhase, int currentIndex, int lastWinPhase, int lastWinIndex)
{
    if (lastWinIndex < 0) return currentPhase == 0 ? 1 : 9999;
    return (currentPhase - lastWinPhase) * CHAIN_PHASE_PLAYER + ((CHAIN_PHASE_PLAYER + currentIndex - lastWinIndex) % CHAIN_PHASE_PLAYER);
}

void static YbtcMiner(const CChainParams& chainparams)
{
    LogPrintf("YbtcMiner started on CPU \n");

    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("ybtc-miner");

    unsigned int nExtraNonce = 0;
    std::shared_ptr<CReserveScript> coinbaseScript;
    if (::vpwallets.size() > 0) {
        GetMainSignals().ScriptForMining(coinbaseScript);
    }


    std::mutex m_cs;

    // Wait for other processes ready by sleeping
    std::this_thread::sleep_for(std::chrono::milliseconds(CHAIN_BLOCK_INTERVAL));
    getMinerAddress();
    LogPrintf("CASINOMINER --- initial miner address is %s\n", HexStr(minerAddress));

    try {
        // Throw an error if no script was provided.  This can happen
        // due to some internal error but also if the keypool is empty.
        // In the latter case, already the pointer is NULL.
        if (!coinbaseScript || coinbaseScript->reserveScript.empty())
            throw std::runtime_error("No coinbase script available (mining requires a wallet)");

        auto timedHeight = -999999;
        int64_t nTimeStart = GetTimeMicros();
        bool abnormalSkip = false;
        auto lastInactive = -999999;

        auto currentPhase = 0;
        auto currentIndex = CHAIN_PHASE_PLAYER - 1;

        auto lastWinPhase = -1;
        auto lastWinIndex = -1;
        auto checkedWinPhase = -1;

        while (true) {
            //
            // Create new block
            //
            CBlockIndex* pindexPrev = chainActive.Tip();

            // Get miner context
            currentPhase = pindexPrev->nHeight / CHAIN_PHASE_SIZE;
            currentIndex = pindexPrev->nHeight % CHAIN_PHASE_SIZE;


            // Abnormal check
            if (timedHeight != pindexPrev->nHeight) {
                timedHeight = pindexPrev->nHeight;
                nTimeStart = GetTimeMicros();
            } else {
                int64_t nTimeEnd = GetTimeMicros();
                int64_t nTimeCheck = nTimeEnd - nTimeStart;
                if (nTimeCheck * 0.000001 > 4 * CHAIN_BLOCK_INTERVAL * whenJumpAbnormal(currentPhase, currentIndex, lastWinPhase, lastWinIndex)) {
                    abnormalSkip = true;
                }
            }

/* jyan
            if (lastInactive != (int)currentIndex) {
                LogPrintf("\n\nCASINOMINER ---  phase %d tip index %d\n", currentPhase, currentIndex);
            }
*/

            if (currentPhase >= 0) {
                //LogPrintf("God is creating Adam and Eve \n");
                minerIndex = CHAIN_PHASE_PLAYER - 1;
                lastWinPhase = 0;
                lastWinIndex = minerIndex;
                std::this_thread::sleep_for(std::chrono::milliseconds(CHAIN_BLOCK_INTERVAL));

            } else if (abnormalSkip) {
                LogPrintf("CASINOMINER ---  nobody mine. I would dig more :( :( : ( :( :) :) :) :) \n");
                //lastWinPhase = currentPhase;
                //lastWinIndex = currentIndex;
                //sleep 2 seconds to leave forbiden zone
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                // reset abnormal status
                abnormalSkip = false;

            } else if (IsNextMiner(currentPhase, currentIndex, &checkedWinPhase)) {
                lastWinPhase = currentPhase;
                lastWinIndex = currentIndex;
                LogPrintf("CASINOMINER ---  moneyMe ******************** \n");
                std::this_thread::sleep_for(std::chrono::milliseconds(CHAIN_BLOCK_INTERVAL));

            } else {
                if (lastInactive != (int)currentIndex) {
                    lastInactive = currentIndex;
                    LogPrintf("CASINOMINER --- inactive \n");
                }
                // No permission to mine, sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(CHAIN_BLOCK_INTERVAL));
                continue;
            }

            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript->reserveScript));
            if (!pblocktemplate.get()) {
                LogPrintf("Error in YbtcMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }

            CBlock* pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            // jyan LogPrintf("YbtcMiner mining   with %u transactions in block (%u bytes) \n", pblock->vtx.size(),
            // jyan     ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));


            if (ProcessBlockFound(pblock, chainparams)) {
            }

            if (chainparams.MineBlocksOnDemand()) {
                throw boost::thread_interrupted();
            }
        }
    } catch (const boost::thread_interrupted&) {
        LogPrintf("YbtcMiner terminated\n");
        throw;
    } catch (const std::runtime_error& e) {
        LogPrintf("YbtcMiner runtime error: %s\n", e.what());
        return;
    }
}


static boost::thread_group* minerThreads = NULL;
void GenerateYbtcs(bool fGenerate, int nThreads, const CChainParams& chainparams)
{
    if (nThreads < 0)
        nThreads = GetNumCores();

    if (minerThreads != NULL) {
        minerThreads->interrupt_all();
        minerThreads->join_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();

    {
        for (int i = 0; i < nThreads; i++) {
            LogPrintf("YbtcMiner CPU, thread=%d!\n", i);
            minerThreads->create_thread(boost::bind(&YbtcMiner, boost::cref(chainparams)));
        }
    }
}
#endif
