// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Ybtc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef YBTC_CHAIN_H
#define YBTC_CHAIN_H

#include "arith_uint256.h"
#include "primitives/block.h"
#include "casino.h"
#include "tinyformat.h"
#include "uint256.h"

#include <vector>

/**
 * Maximum amount of time that a block timestamp is allowed to exceed the
 * current network-adjusted time before the block will be accepted.
 */
static const int64_t MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60;

/**
 * CASINO 
 */
static const uint32_t CHAIN_PHASE_SIZE = 8;
static const uint32_t CHAIN_PHASE_PLAYER = 2;
static const uint32_t CHAIN_BLOCK_INTERVAL = 1;

// static const std::string GENESIS_CONTRACT_ADDRESS_BTC="13vCd93ALeZMrRtm8Rnv4WuixKA7opUWV2";
static const std::string GENESIS_CONTRACT_ADDRESS_BTC="miS9vC899fzcdYNNqzmHtS83pJkphxAvYZ";
static const std::string GENESIS_CONTRACT_ADDRESS_ETH="1fffffffffffffffffffffffffffffffffffffff";


static const std::string CASINO_GETTOTALPLAYER = "0113ea5b";
static const std::string CASINO_ISWINNERME = "43b70f13";
static const std::string CASINO_SETNEXTWINNERS = "39380281";
static const std::string CASINO_BALANCEOF = "722713f7";
static const std::string CASINO_REFILL = "538e0759";
static const std::string CASINO_GETWINNERSEED = "74707c8d";
        

static const std::string GENESIS_CONTRACT_CODE="608060405234801561001057600080fd5b5060006001819055503373ffffffffffffffffffffffffffffffffffffffff16600260006101008110151561004157fe5b0181905550600061020260006101008110151561005a57fe5b01819055506301406f4061010260006101008110151561007657fe5b01819055506108988061008a6000396000f3006080604052600436106100ba576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680630113ea5b146100bf57806318160ddd146100ea578063205e6d2a14610115578063393802811461015657806343b70f13146101a55780634903b0d1146101e6578063538e075914610227578063722713f71461025257806374707c8d1461027d578063a2fb1175146102be578063da246ba9146102ff578063edf26d9b1461032a575b600080fd5b3480156100cb57600080fd5b506100d461036b565b6040518082815260200191505060405180910390f35b3480156100f657600080fd5b506100ff6103b9565b6040518082815260200191505060405180910390f35b34801561012157600080fd5b50610140600480360381019080803590602001909291905050506103bf565b6040518082815260200191505060405180910390f35b34801561016257600080fd5b5061018b60048036038101908080359060200190929190803590602001909291905050506103da565b604051808215151515815260200191505060405180910390f35b3480156101b157600080fd5b506101d06004803603810190808035906020019092919050505061051d565b6040518082815260200191505060405180910390f35b3480156101f257600080fd5b50610211600480360381019080803590602001909291905050506105d4565b6040518082815260200191505060405180910390f35b34801561023357600080fd5b5061023c6105f0565b6040518082815260200191505060405180910390f35b34801561025e57600080fd5b5061026761071c565b6040518082815260200191505060405180910390f35b34801561028957600080fd5b506102a860048036038101908080359060200190929190505050610791565b6040518082815260200191505060405180910390f35b3480156102ca57600080fd5b506102e9600480360381019080803590602001909291905050506107dc565b6040518082815260200191505060405180910390f35b34801561030b57600080fd5b506103146107f8565b6040518082815260200191505060405180910390f35b34801561033657600080fd5b50610355600480360381019080803590602001909291905050506107fe565b6040518082815260200191505060405180910390f35b6000806000809150600090505b6101008110156103b1576000610102826101008110151561039557fe5b015411156103a65781806001019250505b806001019050610378565b819250505090565b60005481565b610302816080811015156103cf57fe5b016000915090505481565b600080600080600080600154881415156103f357600080fd5b60009450600093506000925060018060008282540192505081905550608060015481151561041d57fe5b069150600090505b6101008110156104f7576000610102826101008110151561044257fe5b0154141561044f576104ec565b6001610102826101008110151561046257fe5b01600082825403925050819055506001840193506104808488610819565b156104eb57806102028660028502016101008110151561049c57fe5b018190555061ffff600282610100811015156104b457fe5b015416831892506001850194506002851015156104ea5782610302836080811015156104dc57fe5b018190555060019550610512565b5b5b806001019050610425565b826103028360808110151561050857fe5b0181905550600195505b505050505092915050565b6000806000806000925060015485118061053c57506001546080860111155b15610549578293506105cc565b3373ffffffffffffffffffffffffffffffffffffffff169150600090505b60028110156105c85761020281600260808881151561058257fe5b0602016101008110151561059257fe5b0154925081600284610100811015156105a757fe5b015414156105bd576001810192508293506105cc565b806001019050610567565b8293505b505050919050565b61010281610100811015156105e557fe5b016000915090505481565b60008060008060006301406f40935061010092503373ffffffffffffffffffffffffffffffffffffffff169150600090505b6101008110156106be57816002826101008110151561063d57fe5b01541415610680576014610102826101008110151561065857fe5b0160008282540192505081905550610102816101008110151561067757fe5b01549450610715565b83610102826101008110151561069257fe5b015410156106b35780925061010281610100811015156106ae57fe5b015493505b806001019050610622565b61010083101561071057601461010284610100811015156106db57fe5b018190555081600284610100811015156106f157fe5b0181905550610102836101008110151561070757fe5b01549450610715565b600094505b5050505090565b60008060003373ffffffffffffffffffffffffffffffffffffffff169150600090505b61010081101561078757816002826101008110151561075a57fe5b0154141561077c57610102816101008110151561077357fe5b0154925061078c565b80600101905061073f565b600092505b505090565b60006001548211806107a857506001546080830111155b156107b657600090506107d7565b6103026080838115156107c557fe5b066080811015156107d257fe5b015490505b919050565b61020281610100811015156107ed57fe5b016000915090505481565b60015481565b6002816101008110151561080e57fe5b016000915090505481565b600080600080849250600091505b600282101561085e5761ffff83169050808614156108485760019350610863565b6020839060020a90049250816001019150610827565b600093505b505050929150505600a165627a7a72305820d7c440ad74bb0f6972551b9239bbd4a63983e82c8a41ba2a7e0af0c6686a682d0029";


/**
 * Timestamp window used as a grace period by code that compares external
 * timestamps (such as timestamps passed to RPCs, or wallet key creation times)
 * to block timestamps. This should be set at least as high as
 * MAX_FUTURE_BLOCK_TIME.
 */
static const int64_t TIMESTAMP_WINDOW = MAX_FUTURE_BLOCK_TIME;

class CBlockFileInfo
{
public:
    unsigned int nBlocks;      //!< number of blocks stored in file
    unsigned int nSize;        //!< number of used bytes of block file
    unsigned int nUndoSize;    //!< number of used bytes in the undo file
    unsigned int nHeightFirst; //!< lowest height of block in file
    unsigned int nHeightLast;  //!< highest height of block in file
    uint64_t nTimeFirst;       //!< earliest time of block in file
    uint64_t nTimeLast;        //!< latest time of block in file

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(nBlocks));
        READWRITE(VARINT(nSize));
        READWRITE(VARINT(nUndoSize));
        READWRITE(VARINT(nHeightFirst));
        READWRITE(VARINT(nHeightLast));
        READWRITE(VARINT(nTimeFirst));
        READWRITE(VARINT(nTimeLast));
    }

     void SetNull() {
         nBlocks = 0;
         nSize = 0;
         nUndoSize = 0;
         nHeightFirst = 0;
         nHeightLast = 0;
         nTimeFirst = 0;
         nTimeLast = 0;
     }

     CBlockFileInfo() {
         SetNull();
     }

     std::string ToString() const;

     /** update statistics (does not update nSize) */
     void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn) {
         if (nBlocks==0 || nHeightFirst > nHeightIn)
             nHeightFirst = nHeightIn;
         if (nBlocks==0 || nTimeFirst > nTimeIn)
             nTimeFirst = nTimeIn;
         nBlocks++;
         if (nHeightIn > nHeightLast)
             nHeightLast = nHeightIn;
         if (nTimeIn > nTimeLast)
             nTimeLast = nTimeIn;
     }
};

struct CDiskBlockPos
{
    int nFile;
    unsigned int nPos;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(nFile));
        READWRITE(VARINT(nPos));
    }

    CDiskBlockPos() {
        SetNull();
    }

    CDiskBlockPos(int nFileIn, unsigned int nPosIn) {
        nFile = nFileIn;
        nPos = nPosIn;
    }

    friend bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

    friend bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return !(a == b);
    }

    void SetNull() { nFile = -1; nPos = 0; }
    bool IsNull() const { return (nFile == -1); }

    std::string ToString() const
    {
        return strprintf("CBlockDiskPos(nFile=%i, nPos=%i)", nFile, nPos);
    }

};

enum BlockStatus: uint32_t {
    //! Unused.
    BLOCK_VALID_UNKNOWN      =    0,

    //! Parsed, version ok, hash satisfies claimed PoW, 1 <= vtx count <= max, timestamp not in future
    BLOCK_VALID_HEADER       =    1,

    //! All parent headers found, difficulty matches, timestamp >= median previous, checkpoint. Implies all parents
    //! are also at least TREE.
    BLOCK_VALID_TREE         =    2,

    /**
     * Only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids,
     * sigops, size, merkle root. Implies all parents are at least TREE but not necessarily TRANSACTIONS. When all
     * parent blocks also have TRANSACTIONS, CBlockIndex::nChainTx will be set.
     */
    BLOCK_VALID_TRANSACTIONS =    3,

    //! Outputs do not overspend inputs, no double spends, coinbase output ok, no immature coinbase spends, BIP30.
    //! Implies all parents are also at least CHAIN.
    BLOCK_VALID_CHAIN        =    4,

    //! Scripts & signatures ok. Implies all parents are also at least SCRIPTS.
    BLOCK_VALID_SCRIPTS      =    5,

    //! All validity bits.
    BLOCK_VALID_MASK         =   BLOCK_VALID_HEADER | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
                                 BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,

    BLOCK_HAVE_DATA          =    8, //!< full block available in blk*.dat
    BLOCK_HAVE_UNDO          =   16, //!< undo data available in rev*.dat
    BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,

    BLOCK_FAILED_VALID       =   32, //!< stage after last reached validness failed
    BLOCK_FAILED_CHILD       =   64, //!< descends from failed block
    BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,

    BLOCK_OPT_WITNESS       =   128, //!< block data in blk*.data was received with a witness-enforcing client
};

/** The block chain is a tree shaped structure starting with the
 * genesis block at the root, with each block potentially having multiple
 * candidates to be the next block. A blockindex may have multiple pprev pointing
 * to it, but at most one of them can be part of the currently active branch.
 */
class CBlockIndex
{
public:
    //! pointer to the hash of the block, if any. Memory is owned by this CBlockIndex
    const uint256* phashBlock;

    //! pointer to the index of the predecessor of this block
    CBlockIndex* pprev;

    //! pointer to the index of some further predecessor of this block
    CBlockIndex* pskip;

    //! height of the entry in the chain. The genesis block has height 0
    int nHeight;

    //! Which # file this block is stored in (blk?????.dat)
    int nFile;

    //! Byte offset within blk?????.dat where this block's data is stored
    unsigned int nDataPos;

    //! Byte offset within rev?????.dat where this block's undo data is stored
    unsigned int nUndoPos;

    //! (memory only) Total amount of work (expected number of hashes) in the chain up to and including this block
    arith_uint256 nChainWork;

    //! Number of transactions in this block.
    //! Note: in a potential headers-first mode, this number cannot be relied upon
    unsigned int nTx;

    //! (memory only) Number of transactions in the chain up to and including this block.
    //! This value will be non-zero only if and only if transactions for this block and all its parents are available.
    //! Change to 64-bit type when necessary; won't happen before 2030
    unsigned int nChainTx;

    //! Verification status of this block. See enum BlockStatus
    unsigned int nStatus;

    //! block header
    int nVersion;
    uint256 hashMerkleRoot;
    uint256 hashStateRoot;
    unsigned int nTime;
    unsigned int nBits;

    //! (memory only) Sequential id assigned to distinguish order in which blocks are received.
    int32_t nSequenceId;

    //! (memory only) Maximum nTime in the chain upto and including this block.
    unsigned int nTimeMax;

    void SetNull()
    {
        phashBlock = nullptr;
        pprev = nullptr;
        pskip = nullptr;
        nHeight = 0;
        nFile = 0;
        nDataPos = 0;
        nUndoPos = 0;
        nChainWork = arith_uint256();
        nTx = 0;
        nChainTx = 0;
        nStatus = 0;
        nSequenceId = 0;
        nTimeMax = 0;

        nVersion       = 0;
        hashMerkleRoot = uint256();
        hashStateRoot = uint256();
        nTime          = 0;
        nBits          = 0;
    }

    CBlockIndex()
    {
        SetNull();
    }

    CBlockIndex(const CBlockHeader& block)
    {
        SetNull();

        nVersion       = block.nVersion;
        hashMerkleRoot = block.hashMerkleRoot;
        hashStateRoot  = block.hashStateRoot;
        nTime          = block.nTime;
        nBits          = block.nBits;
    }

    CDiskBlockPos GetBlockPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos  = nDataPos;
        }
        return ret;
    }

    CDiskBlockPos GetUndoPos() const {
        CDiskBlockPos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos  = nUndoPos;
        }
        return ret;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        if (pprev)
            block.hashPrevBlock = pprev->GetBlockHash();
        block.hashMerkleRoot = hashMerkleRoot;
        block.hashStateRoot  = hashStateRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nHeight        = nHeight;
        return block;
    }

    uint256 GetBlockHash() const
    {
        return *phashBlock;
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    int64_t GetBlockTimeMax() const
    {
        return (int64_t)nTimeMax;
    }

    enum { nMedianTimeSpan=11 };

    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
            *(--pbegin) = pindex->GetBlockTime();

        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }

    std::string ToString() const
    {
        return strprintf("CBlockIndex(pprev=%p, nHeight=%d, merkle=%s, state=%s hashBlock=%s)",
            pprev, nHeight,
            hashMerkleRoot.ToString(),
            hashStateRoot.ToString(),
            GetBlockHash().ToString());
    }

    //! Check whether this block index entry is valid up to the passed validity level.
    bool IsValid(enum BlockStatus nUpTo = BLOCK_VALID_TRANSACTIONS) const
    {
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        return ((nStatus & BLOCK_VALID_MASK) >= nUpTo);
    }

    //! Raise the validity level of this block index entry.
    //! Returns true if the validity was changed.
    bool RaiseValidity(enum BlockStatus nUpTo)
    {
        assert(!(nUpTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        if ((nStatus & BLOCK_VALID_MASK) < nUpTo) {
            nStatus = (nStatus & ~BLOCK_VALID_MASK) | nUpTo;
            return true;
        }
        return false;
    }

    //! Build the skiplist pointer for this entry.
    void BuildSkip();

    //! Efficiently find an ancestor of this block.
    CBlockIndex* GetAncestor(int height);
    const CBlockIndex* GetAncestor(int height) const;
};

arith_uint256 GetBlockProof(const CBlockIndex& block);
/** Return the time it would take to redo the work difference between from and to, assuming the current hashrate corresponds to the difficulty at tip, in seconds. */
int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params&);
/** Find the forking point between two chain tips. */
const CBlockIndex* LastCommonAncestor(const CBlockIndex* pa, const CBlockIndex* pb);


/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex : public CBlockIndex
{
public:
    uint256 hashPrev;

    CDiskBlockIndex() {
        hashPrev = uint256();
    }

    explicit CDiskBlockIndex(const CBlockIndex* pindex) : CBlockIndex(*pindex) {
        hashPrev = (pprev ? pprev->GetBlockHash() : uint256());
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int _nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(VARINT(_nVersion));

        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nStatus));
        READWRITE(VARINT(nTx));
        if (nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
            READWRITE(VARINT(nFile));
        if (nStatus & BLOCK_HAVE_DATA)
            READWRITE(VARINT(nDataPos));
        if (nStatus & BLOCK_HAVE_UNDO)
            READWRITE(VARINT(nUndoPos));

        // block header
        READWRITE(this->nVersion);
        READWRITE(hashPrev);
        READWRITE(hashMerkleRoot);
        READWRITE(hashStateRoot);
        READWRITE(nTime);
        READWRITE(nBits);
    }

    uint256 GetBlockHash() const
    {
        CBlockHeader block;
        block.nVersion        = nVersion;
        block.hashPrevBlock   = hashPrev;
        block.hashMerkleRoot  = hashMerkleRoot;
        block.hashStateRoot   = hashStateRoot;
        block.nTime           = nTime;
        block.nBits           = nBits;
        block.nHeight         = nHeight;
        return block.GetHash();
    }


    std::string ToString() const
    {
        std::string str = "CDiskBlockIndex(";
        str += CBlockIndex::ToString();
        str += strprintf("\n                hashBlock=%s, hashPrev=%s)",
            GetBlockHash().ToString(),
            hashPrev.ToString());
        return str;
    }
};

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    std::vector<CBlockIndex*> vChain;

public:
    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex *Genesis() const {
        return vChain.size() > 0 ? vChain[0] : nullptr;
    }

    /** Returns the index entry for the tip of this chain, or nullptr if none. */
    CBlockIndex *Tip() const {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : nullptr;
    }

    /** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
    CBlockIndex *operator[](int nHeight) const {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return nullptr;
        return vChain[nHeight];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.vChain.size() - 1] == b.vChain[b.vChain.size() - 1];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pindex) const {
        return (*this)[pindex->nHeight] == pindex;
    }

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pindex) const {
        if (Contains(pindex))
            return (*this)[pindex->nHeight + 1];
        else
            return nullptr;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->nHeight : -1. */
    int Height() const {
        return vChain.size() - 1;
    }

    /** Set/initialize a chain with a given tip. */
    void SetTip(CBlockIndex *pindex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pindex = nullptr) const;

    /** Find the last common block between this chain and a block index entry. */
    const CBlockIndex *FindFork(const CBlockIndex *pindex) const;

    /** Find the earliest block with timestamp equal or greater than the given. */
    CBlockIndex* FindEarliestAtLeast(int64_t nTime) const;
};

#endif // YBTC_CHAIN_H
