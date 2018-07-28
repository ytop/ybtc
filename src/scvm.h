
#ifndef FABCOIN_SCVM_HPP
#define FABCOIN_SCVM_HPP

#include "scdb.h"
#include "scstate.h"
#include "scaccount.h"

namespace sc
{

class owning_bytes_ref : public vector_ref<byte const>
{
public:
    owning_bytes_ref() = default;

    /// @param _bytes  The buffer.
    /// @param _begin  The index of the first referenced byte.
    /// @param _size   The number of referenced bytes.
    owning_bytes_ref(bytes&& _bytes, size_t _begin, size_t _size) : m_bytes(std::move(_bytes))
    {
        // Set the reference *after* the buffer is moved to avoid
        // pointer invalidation.
        retarget(&m_bytes[_begin], _size);
    }

    owning_bytes_ref(owning_bytes_ref const&) = delete;
    owning_bytes_ref(owning_bytes_ref&&) = default;
    owning_bytes_ref& operator=(owning_bytes_ref const&) = delete;
    owning_bytes_ref& operator=(owning_bytes_ref&&) = default;

private:
    bytes m_bytes;
};




// ============ Instruction ======

#define INIT_CASES         \
    if (!m_caseInit) {     \
        m_caseInit = true; \
        return;            \
    }
#define DO_CASES            \
    for (;;) {              \
        fetchInstruction(); \
        switch (m_OP) {
#define CASE(name) case Instruction::name:
#define NEXT \
    ++m_PC;  \
    break;
#define CONTINUE continue;
#define BREAK return;
#define DEFAULT default:
#define WHILE_CASES \
    }               \
    }

/// Virtual machine bytecode instruction.
enum class Instruction : uint8_t {
    STOP = 0x00, ///< halts execution
    ADD,         ///< addition operation
    MUL,         ///< mulitplication operation
    SUB,         ///< subtraction operation
    DIV,         ///< integer division operation
    SDIV,        ///< signed integer division operation
    MOD,         ///< modulo remainder operation
    SMOD,        ///< signed modulo remainder operation
    ADDMOD,      ///< unsigned modular addition
    MULMOD,      ///< unsigned modular multiplication
    EXP,         ///< exponential operation
    SIGNEXTEND,  ///< extend length of signed integer

    LT = 0x10, ///< less-than comparision
    GT,        ///< greater-than comparision
    SLT,       ///< signed less-than comparision
    SGT,       ///< signed greater-than comparision
    EQ,        ///< equality comparision
    ISZERO,    ///< simple not operator
    AND,       ///< bitwise AND operation
    OR,        ///< bitwise OR operation
    XOR,       ///< bitwise XOR operation
    NOT,       ///< bitwise NOT opertation
    BYTE,      ///< retrieve single byte from word

    SHA3 = 0x20, ///< compute SHA3-256 hash

    ADDRESS = 0x30, ///< get address of currently executing account
    BALANCE,        ///< get balance of the given account
    ORIGIN,         ///< get execution origination address
    CALLER,         ///< get caller address
    CALLVALUE,      ///< get deposited value by the instruction/transaction responsible for this execution
    CALLDATALOAD,   ///< get input data of current environment
    CALLDATASIZE,   ///< get size of input data in current environment
    CALLDATACOPY,   ///< copy input data in current environment to memory
    CODESIZE,       ///< get size of code running in current environment
    CODECOPY,       ///< copy code running in current environment to memory
    GASPRICE,       ///< get price of gas in current environment
    EXTCODESIZE,    ///< get external code size (from another contract)
    EXTCODECOPY,    ///< copy external code (from another contract)

    BLOCKHASH = 0x40, ///< get hash of most recent complete block
    COINBASE,         ///< get the block's coinbase address
    TIMESTAMP,        ///< get the block's timestamp
    NUMBER,           ///< get the block's number
    DIFFICULTY,       ///< get the block's difficulty
    GASLIMIT,         ///< get the block's gas limit

    JUMPTO = 0x4a, ///< alter the program counter to a jumpdest
    JUMPIF,        ///< conditionally alter the program counter
    JUMPV,         ///< alter the program counter to a jumpdest
    JUMPSUB,       ///< alter the program counter to a beginsub
    JUMPSUBV,      ///< alter the program counter to a beginsub
    RETURNSUB,     ///< return to subroutine jumped from

    POP = 0x50, ///< remove item from stack
    MLOAD,      ///< load word from memory
    MSTORE,     ///< save word to memory
    MSTORE8,    ///< save byte to memory
    SLOAD,      ///< load word from storage
    SSTORE,     ///< save word to storage
    JUMP,       ///< alter the program counter to a jumpdest
    JUMPI,      ///< conditionally alter the program counter
    PC,         ///< get the program counter
    MSIZE,      ///< get the size of active memory
    GAS,        ///< get the amount of available gas
    JUMPDEST,   ///< set a potential jump destination
    BEGINSUB,   ///< set a potential jumpsub destination
    BEGINDATA,  ///< begine the data section

    PUSH1 = 0x60, ///< place 1 byte item on stack
    PUSH2,        ///< place 2 byte item on stack
    PUSH3,        ///< place 3 byte item on stack
    PUSH4,        ///< place 4 byte item on stack
    PUSH5,        ///< place 5 byte item on stack
    PUSH6,        ///< place 6 byte item on stack
    PUSH7,        ///< place 7 byte item on stack
    PUSH8,        ///< place 8 byte item on stack
    PUSH9,        ///< place 9 byte item on stack
    PUSH10,       ///< place 10 byte item on stack
    PUSH11,       ///< place 11 byte item on stack
    PUSH12,       ///< place 12 byte item on stack
    PUSH13,       ///< place 13 byte item on stack
    PUSH14,       ///< place 14 byte item on stack
    PUSH15,       ///< place 15 byte item on stack
    PUSH16,       ///< place 16 byte item on stack
    PUSH17,       ///< place 17 byte item on stack
    PUSH18,       ///< place 18 byte item on stack
    PUSH19,       ///< place 19 byte item on stack
    PUSH20,       ///< place 20 byte item on stack
    PUSH21,       ///< place 21 byte item on stack
    PUSH22,       ///< place 22 byte item on stack
    PUSH23,       ///< place 23 byte item on stack
    PUSH24,       ///< place 24 byte item on stack
    PUSH25,       ///< place 25 byte item on stack
    PUSH26,       ///< place 26 byte item on stack
    PUSH27,       ///< place 27 byte item on stack
    PUSH28,       ///< place 28 byte item on stack
    PUSH29,       ///< place 29 byte item on stack
    PUSH30,       ///< place 30 byte item on stack
    PUSH31,       ///< place 31 byte item on stack
    PUSH32,       ///< place 32 byte item on stack

    DUP1 = 0x80, ///< copies the highest item in the stack to the top of the stack
    DUP2,        ///< copies the second highest item in the stack to the top of the stack
    DUP3,        ///< copies the third highest item in the stack to the top of the stack
    DUP4,        ///< copies the 4th highest item in the stack to the top of the stack
    DUP5,        ///< copies the 5th highest item in the stack to the top of the stack
    DUP6,        ///< copies the 6th highest item in the stack to the top of the stack
    DUP7,        ///< copies the 7th highest item in the stack to the top of the stack
    DUP8,        ///< copies the 8th highest item in the stack to the top of the stack
    DUP9,        ///< copies the 9th highest item in the stack to the top of the stack
    DUP10,       ///< copies the 10th highest item in the stack to the top of the stack
    DUP11,       ///< copies the 11th highest item in the stack to the top of the stack
    DUP12,       ///< copies the 12th highest item in the stack to the top of the stack
    DUP13,       ///< copies the 13th highest item in the stack to the top of the stack
    DUP14,       ///< copies the 14th highest item in the stack to the top of the stack
    DUP15,       ///< copies the 15th highest item in the stack to the top of the stack
    DUP16,       ///< copies the 16th highest item in the stack to the top of the stack

    SWAP1 = 0x90, ///< swaps the highest and second highest value on the stack
    SWAP2,        ///< swaps the highest and third highest value on the stack
    SWAP3,        ///< swaps the highest and 4th highest value on the stack
    SWAP4,        ///< swaps the highest and 5th highest value on the stack
    SWAP5,        ///< swaps the highest and 6th highest value on the stack
    SWAP6,        ///< swaps the highest and 7th highest value on the stack
    SWAP7,        ///< swaps the highest and 8th highest value on the stack
    SWAP8,        ///< swaps the highest and 9th highest value on the stack
    SWAP9,        ///< swaps the highest and 10th highest value on the stack
    SWAP10,       ///< swaps the highest and 11th highest value on the stack
    SWAP11,       ///< swaps the highest and 12th highest value on the stack
    SWAP12,       ///< swaps the highest and 13th highest value on the stack
    SWAP13,       ///< swaps the highest and 14th highest value on the stack
    SWAP14,       ///< swaps the highest and 15th highest value on the stack
    SWAP15,       ///< swaps the highest and 16th highest value on the stack
    SWAP16,       ///< swaps the highest and 17th highest value on the stack

    LOG0 = 0xa0, ///< Makes a log entry; no topics.
    LOG1,        ///< Makes a log entry; 1 topic.
    LOG2,        ///< Makes a log entry; 2 topics.
    LOG3,        ///< Makes a log entry; 3 topics.
    LOG4,        ///< Makes a log entry; 4 topics.

    // these are generated by the interpreter - should never be in user code
    PUSHC = 0xac, ///< push value from constant pool
    JUMPC,        ///< alter the program counter - pre-verified
    JUMPCI,       ///< conditionally alter the program counter - pre-verified
    BAD,          ///< placed to force invalid instruction exception

    CREATE = 0xf0,     ///< create a new account with associated code
    CALL,              ///< message-call into an account
    CALLCODE,          ///< message-call with another account's code only
    RETURN,            ///< halt execution returning output data
    DELEGATECALL,      ///< like CALLCODE but keeps caller's value and sender
    STATICCALL = 0xfa, ///< Like CALL but does not allow state modification.
    REVERT = 0xfd,     ///< stop execution and revert state changes, without consuming all provided gas
    SUICIDE = 0xff     ///< halt execution and register account for later deletion
};

enum class Tier : unsigned {
    Zero = 0, // 0, Zero
    Base,     // 2, Quick
    VeryLow,  // 3, Fastest
    Low,      // 5, Fast
    Mid,      // 8, Mid
    High,     // 10, Slow
    Ext,      // 20, Ext
    Special,  // multiparam or otherwise special
    Invalid   // Invalid.
};


// ====== EVMSchedule =======
struct EVMSchedule {
    EVMSchedule() : tierStepGas(std::array<unsigned, 8>{
                        {0, 2, 3, 5, 8, 10, 20, 0}}) {}
    EVMSchedule(bool _efcd, bool _hdc, unsigned const& _txCreateGas) : exceptionalFailedCodeDeposit(_efcd), haveDelegateCall(_hdc), tierStepGas(std::array<unsigned, 8>{
                                                                                                                                        {0, 2, 3, 5, 8, 10, 20, 0}}),
                                                                       txCreateGas(_txCreateGas) {}
    bool exceptionalFailedCodeDeposit = true;
    bool haveDelegateCall = true;
    bool eip150Mode = false;
    bool eip158Mode = false;
    std::array<unsigned, 8> tierStepGas;
    unsigned expGas = 10;
    unsigned expByteGas = 10;
    unsigned sha3Gas = 30;
    unsigned sha3WordGas = 6;
    unsigned sloadGas = 50;
    unsigned sstoreSetGas = 20000;
    unsigned sstoreResetGas = 5000;
    unsigned sstoreRefundGas = 15000;
    unsigned jumpdestGas = 1;
    unsigned logGas = 375;
    unsigned logDataGas = 8;
    unsigned logTopicGas = 375;
    unsigned createGas = 32000;
    unsigned callGas = 40;
    unsigned callStipend = 2300;
    unsigned callValueTransferGas = 9000;
    unsigned callNewAccountGas = 25000;
    unsigned suicideRefundGas = 24000;
    unsigned memoryGas = 3;
    unsigned quadCoeffDiv = 512;
    unsigned createDataGas = 200;
    unsigned txGas = 21000;
    unsigned txCreateGas = 53000;
    unsigned txDataZeroGas = 4;
    unsigned txDataNonZeroGas = 68;
    unsigned copyGas = 3;

    unsigned extcodesizeGas = 20;
    unsigned extcodecopyGas = 20;
    unsigned balanceGas = 20;
    unsigned suicideGas = 0;
    unsigned maxCodeSize = unsigned(-1);

    bool staticCallDepthLimit() const
    {
        return !eip150Mode;
    }
    bool suicideChargesNewAccountGas() const
    {
        return eip150Mode;
    }
    bool emptinessIsNonexistence() const
    {
        return eip158Mode;
    }
    bool zeroValueTransferChargesNewAccountGas() const
    {
        return !eip158Mode;
    }
};


// ======== EnvInfo =======
class VM;
class ExtVMFace;


using LastHashes = std::vector<h256>;
static const EVMSchedule DefaultSchedule = EVMSchedule();


struct LogEntry {
    LogEntry() {}
    LogEntry(Address const& _address, h256s const& _ts, bytes&& _d) : address(_address), topics(_ts), data(std::move(_d)) {}

    Address address;
    h256s topics;
    bytes data;
};


using LogEntries = std::vector<LogEntry>;

using OnOpFunc = std::function<void(uint64_t /*steps*/, uint64_t /* PC */, Instruction /*instr*/, bigint /*newMemSize*/, bigint /*gasCost*/, bigint /*gas*/, VM*, ExtVMFace const*)>;

struct SubState {
    std::set<Address> suicides; ///< Any accounts that have suicided.
    LogEntries logs;            ///< Any logs.
    u256 refunds;               ///< Refund counter of SSTORE nonzero->zero.

    SubState& operator+=(SubState const& _s)
    {
        for (auto s : _s.suicides)
            suicides.insert(s);
        refunds += _s.refunds;
        //logs += _s.logs;
        for (auto sl : _s.logs)
            logs.emplace_back(sl);
        return *this;
    }

    void clear()
    {
        suicides.clear();
        logs.clear();
        refunds = 0;
    }
};

struct CallParameters {
    Address senderAddress;
    Address codeAddress;
    Address receiveAddress;
    u256 valueTransfer;
    u256 apparentValue;
    u256 gas;
    bytesConstRef data;
    OnOpFunc onOp;
};


/**
    * @brief Interface and null implementation of the class for specifying VM externalities.
    */

// ====== VM.h  =======


struct InstructionMetric {
    Tier gasPriceTier;
    int args;
    int ret;
};


/**
    */
class VMFace
{
public:
    VMFace() = default;
    virtual ~VMFace() = default;
    VMFace(VMFace const&) = delete;
    VMFace& operator=(VMFace const&) = delete;

    /// VM implementation
    virtual owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ctx, OnOpFunc const& _onOp) = 0;
};

class VM : public VMFace
{
public:
    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ctx, OnOpFunc const& _onOp);

    bytes const& memory() const
    {
        return m_mem;
    }
    u256s stack() const
    {
        assert(m_stack <= m_SP + 1);
        return u256s(m_stack, m_SP + 1);
    };

private:
    u256* io_gas = 0;
    uint64_t m_io_gas = 0;
    ExtVMFace* m_ctx = 0;
    OnOpFunc m_onOp;

    static std::array<InstructionMetric, 256> c_metrics;
    static void initMetrics();
    static u256 exp256(u256 _base, u256 _exponent);
    void copyCode(int);
    const void* const* c_jumpTable = 0;
    bool m_caseInit = false;

    typedef void (VM::*MemFnPtr)();
    MemFnPtr m_bounce = 0;
    MemFnPtr m_onFail = 0;
    uint64_t m_nSteps = 0;
    EVMSchedule const* m_schedule = nullptr;

    // return bytes
    owning_bytes_ref m_output;

    // space for memory
    bytes m_mem;

    // space for code and pointer to data
    bytes m_codeSpace;
    byte* m_code = nullptr;

    // space for stack and pointer to data
    u256 m_stackSpace[1025];
    u256* m_stack = m_stackSpace + 1;
    ptrdiff_t stackSize()
    {
        return m_SP - m_stack;
    }

    // space for return stack and pointer to data
    uint64_t m_returnSpace[1025];
    uint64_t* m_return = m_returnSpace + 1;

    // mark PCs with frame size to detect cycles and stack mismatch
    std::vector<size_t> m_frameSize;

    // constant pool
    u256 m_pool[256];

    // interpreter state
    Instruction m_OP;              // current operator
    uint64_t m_PC = 0;             // program counter
    u256* m_SP = m_stack - 1;      // stack pointer
    uint64_t* m_RP = m_return - 1; // return pointer

    // metering and memory state
    uint64_t m_runGas = 0;
    uint64_t m_newMemSize = 0;
    uint64_t m_copyMemSize = 0;

    // initialize interpreter
    void initEntry();
    void optimize();

    // interpreter loop & switch
    void interpretCases();

    // interpreter cases that call out
    void caseCreate();
    bool caseCallSetup(CallParameters*, bytesRef& o_output);
    void caseCall();

    void copyDataToMemory(bytesConstRef _data, u256*& m_SP);
    uint64_t memNeed(u256 _offset, u256 _size);

    void throwOutOfGas();
    void throwBadInstruction();
    void throwBadJumpDestination();
    void throwBadStack(unsigned _size, unsigned _n, unsigned _d);
    void throwRevertInstruction(owning_bytes_ref&& _output);

    void reportStackUse();

    std::vector<uint64_t> m_beginSubs;
    std::vector<uint64_t> m_jumpDests;
    int64_t verifyJumpDest(u256 const& _dest, bool _throw = true);

    int poolConstant(const u256&);

    void onOperation();
    void checkStack(unsigned _n, unsigned _d);
    uint64_t gasForMem(u512 _size);
    void updateIOGas();
    void updateGas();
    void updateMem();
    void logGasMem();
    void fetchInstruction();

    uint64_t decodeJumpDest(const byte* const _code, uint64_t& _pc);
    uint64_t decodeJumpvDest(const byte* const _code, uint64_t& _pc, u256*& _sp);

    template <class T>
    uint64_t toInt63(T v)
    {
        // check for overflow
        if (v > 0x7FFFFFFFFFFFFFFF)
            throwOutOfGas();
        uint64_t w = uint64_t(v);
        return w;
    }
};


// ====== Instruction  =======
struct InstructionInfo {
    std::string name;  ///< The name of the instruction.
    int additional;    ///< Additional items required in memory for this instructions (only for PUSH).
    int args;          ///< Number of items required on the stack for this instruction (and, for the purposes of ret, the number taken from the stack).
    int ret;           ///< Number of items placed (back) on the stack by this instruction, assuming args items were removed.
    bool sideEffects;  ///< false if the only effect on the execution environment (apart from gas usage) is a change to a topmost segment of the stack
    Tier gasPriceTier; ///< Tier for gas pricing.
};


const std::map<std::string, Instruction> c_instructions =
    {
        {"STOP", Instruction::STOP},
        {"ADD", Instruction::ADD},
        {"SUB", Instruction::SUB},
        {"MUL", Instruction::MUL},
        {"DIV", Instruction::DIV},
        {"SDIV", Instruction::SDIV},
        {"MOD", Instruction::MOD},
        {"SMOD", Instruction::SMOD},
        {"EXP", Instruction::EXP},
        {"BNOT", Instruction::NOT},
        {"LT", Instruction::LT},
        {"GT", Instruction::GT},
        {"SLT", Instruction::SLT},
        {"SGT", Instruction::SGT},
        {"EQ", Instruction::EQ},
        {"NOT", Instruction::ISZERO},
        {"AND", Instruction::AND},
        {"OR", Instruction::OR},
        {"XOR", Instruction::XOR},
        {"BYTE", Instruction::BYTE},
        {"ADDMOD", Instruction::ADDMOD},
        {"MULMOD", Instruction::MULMOD},
        {"SIGNEXTEND", Instruction::SIGNEXTEND},
        {"SHA3", Instruction::SHA3},
        {"ADDRESS", Instruction::ADDRESS},
        {"BALANCE", Instruction::BALANCE},
        {"ORIGIN", Instruction::ORIGIN},
        {"CALLER", Instruction::CALLER},
        {"CALLVALUE", Instruction::CALLVALUE},
        {"CALLDATALOAD", Instruction::CALLDATALOAD},
        {"CALLDATASIZE", Instruction::CALLDATASIZE},
        {"CALLDATACOPY", Instruction::CALLDATACOPY},
        {"CODESIZE", Instruction::CODESIZE},
        {"CODECOPY", Instruction::CODECOPY},
        {"GASPRICE", Instruction::GASPRICE},
        {"EXTCODESIZE", Instruction::EXTCODESIZE},
        {"EXTCODECOPY", Instruction::EXTCODECOPY},
        {"BLOCKHASH", Instruction::BLOCKHASH},
        {"COINBASE", Instruction::COINBASE},
        {"TIMESTAMP", Instruction::TIMESTAMP},
        {"NUMBER", Instruction::NUMBER},
        {"DIFFICULTY", Instruction::DIFFICULTY},
        {"GASLIMIT", Instruction::GASLIMIT},
        {"JUMPTO", Instruction::JUMPTO},
        {"JUMPIF", Instruction::JUMPTO},
        {"JUMPSUB", Instruction::JUMPSUB},
        {"JUMPV", Instruction::JUMPV},
        {"JUMPSUBV", Instruction::JUMPSUBV},
        {"RETURNSUB", Instruction::RETURNSUB},
        {"POP", Instruction::POP},
        {"MLOAD", Instruction::MLOAD},
        {"MSTORE", Instruction::MSTORE},
        {"MSTORE8", Instruction::MSTORE8},
        {"SLOAD", Instruction::SLOAD},
        {"SSTORE", Instruction::SSTORE},
        {"JUMP", Instruction::JUMP},
        {"JUMPI", Instruction::JUMPI},
        {"PC", Instruction::PC},
        {"MSIZE", Instruction::MSIZE},
        {"GAS", Instruction::GAS},
        {"BEGIN", Instruction::JUMPDEST},
        {"JUMPDEST", Instruction::JUMPDEST},
        {"BEGINDATA", Instruction::BEGINDATA},
        {"BEGINSUB", Instruction::BEGINSUB},
        {"PUSH1", Instruction::PUSH1},
        {"PUSH2", Instruction::PUSH2},
        {"PUSH3", Instruction::PUSH3},
        {"PUSH4", Instruction::PUSH4},
        {"PUSH5", Instruction::PUSH5},
        {"PUSH6", Instruction::PUSH6},
        {"PUSH7", Instruction::PUSH7},
        {"PUSH8", Instruction::PUSH8},
        {"PUSH9", Instruction::PUSH9},
        {"PUSH10", Instruction::PUSH10},
        {"PUSH11", Instruction::PUSH11},
        {"PUSH12", Instruction::PUSH12},
        {"PUSH13", Instruction::PUSH13},
        {"PUSH14", Instruction::PUSH14},
        {"PUSH15", Instruction::PUSH15},
        {"PUSH16", Instruction::PUSH16},
        {"PUSH17", Instruction::PUSH17},
        {"PUSH18", Instruction::PUSH18},
        {"PUSH19", Instruction::PUSH19},
        {"PUSH20", Instruction::PUSH20},
        {"PUSH21", Instruction::PUSH21},
        {"PUSH22", Instruction::PUSH22},
        {"PUSH23", Instruction::PUSH23},
        {"PUSH24", Instruction::PUSH24},
        {"PUSH25", Instruction::PUSH25},
        {"PUSH26", Instruction::PUSH26},
        {"PUSH27", Instruction::PUSH27},
        {"PUSH28", Instruction::PUSH28},
        {"PUSH29", Instruction::PUSH29},
        {"PUSH30", Instruction::PUSH30},
        {"PUSH31", Instruction::PUSH31},
        {"PUSH32", Instruction::PUSH32},
        {"DUP1", Instruction::DUP1},
        {"DUP2", Instruction::DUP2},
        {"DUP3", Instruction::DUP3},
        {"DUP4", Instruction::DUP4},
        {"DUP5", Instruction::DUP5},
        {"DUP6", Instruction::DUP6},
        {"DUP7", Instruction::DUP7},
        {"DUP8", Instruction::DUP8},
        {"DUP9", Instruction::DUP9},
        {"DUP10", Instruction::DUP10},
        {"DUP11", Instruction::DUP11},
        {"DUP12", Instruction::DUP12},
        {"DUP13", Instruction::DUP13},
        {"DUP14", Instruction::DUP14},
        {"DUP15", Instruction::DUP15},
        {"DUP16", Instruction::DUP16},
        {"SWAP1", Instruction::SWAP1},
        {"SWAP2", Instruction::SWAP2},
        {"SWAP3", Instruction::SWAP3},
        {"SWAP4", Instruction::SWAP4},
        {"SWAP5", Instruction::SWAP5},
        {"SWAP6", Instruction::SWAP6},
        {"SWAP7", Instruction::SWAP7},
        {"SWAP8", Instruction::SWAP8},
        {"SWAP9", Instruction::SWAP9},
        {"SWAP10", Instruction::SWAP10},
        {"SWAP11", Instruction::SWAP11},
        {"SWAP12", Instruction::SWAP12},
        {"SWAP13", Instruction::SWAP13},
        {"SWAP14", Instruction::SWAP14},
        {"SWAP15", Instruction::SWAP15},
        {"SWAP16", Instruction::SWAP16},
        {"LOG0", Instruction::LOG0},
        {"LOG1", Instruction::LOG1},
        {"LOG2", Instruction::LOG2},
        {"LOG3", Instruction::LOG3},
        {"LOG4", Instruction::LOG4},
        {"CREATE", Instruction::CREATE},
        {"CALL", Instruction::CALL},
        {"CALLCODE", Instruction::CALLCODE},
        {"RETURN", Instruction::RETURN},
        {"DELEGATECALL", Instruction::DELEGATECALL},
        {"SUICIDE", Instruction::SUICIDE},

        // these are generated by the interpreter - should never be in user code
        {"PUSHC", Instruction::PUSHC},
        {"JUMPC", Instruction::JUMPC},
        {"JUMPCI", Instruction::JUMPCI}};

static const std::map<Instruction, InstructionInfo> c_instructionInfo =
    {
        //                                               Add,  Args,  Ret,  SideEffects, GasPriceTier
        {Instruction::STOP, {"STOP", 0, 0, 0, true, Tier::Zero}},
        {Instruction::ADD, {"ADD", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::SUB, {"SUB", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::MUL, {"MUL", 0, 2, 1, false, Tier::Low}},
        {Instruction::DIV, {"DIV", 0, 2, 1, false, Tier::Low}},
        {Instruction::SDIV, {"SDIV", 0, 2, 1, false, Tier::Low}},
        {Instruction::MOD, {"MOD", 0, 2, 1, false, Tier::Low}},
        {Instruction::SMOD, {"SMOD", 0, 2, 1, false, Tier::Low}},
        {Instruction::EXP, {"EXP", 0, 2, 1, false, Tier::Special}},
        {Instruction::NOT, {"NOT", 0, 1, 1, false, Tier::VeryLow}},
        {Instruction::LT, {"LT", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::GT, {"GT", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::SLT, {"SLT", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::SGT, {"SGT", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::EQ, {"EQ", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::ISZERO, {"ISZERO", 0, 1, 1, false, Tier::VeryLow}},
        {Instruction::AND, {"AND", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::OR, {"OR", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::XOR, {"XOR", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::BYTE, {"BYTE", 0, 2, 1, false, Tier::VeryLow}},
        {Instruction::ADDMOD, {"ADDMOD", 0, 3, 1, false, Tier::Mid}},
        {Instruction::MULMOD, {"MULMOD", 0, 3, 1, false, Tier::Mid}},
        {Instruction::SIGNEXTEND, {"SIGNEXTEND", 0, 2, 1, false, Tier::Low}},
        {Instruction::SHA3, {"SHA3", 0, 2, 1, false, Tier::Special}},
        {Instruction::ADDRESS, {"ADDRESS", 0, 0, 1, false, Tier::Base}},
        {Instruction::BALANCE, {"BALANCE", 0, 1, 1, false, Tier::Special}},
        {Instruction::ORIGIN, {"ORIGIN", 0, 0, 1, false, Tier::Base}},
        {Instruction::CALLER, {"CALLER", 0, 0, 1, false, Tier::Base}},
        {Instruction::CALLVALUE, {"CALLVALUE", 0, 0, 1, false, Tier::Base}},
        {Instruction::CALLDATALOAD, {"CALLDATALOAD", 0, 1, 1, false, Tier::VeryLow}},
        {Instruction::CALLDATASIZE, {"CALLDATASIZE", 0, 0, 1, false, Tier::Base}},
        {Instruction::CALLDATACOPY, {"CALLDATACOPY", 0, 3, 0, true, Tier::VeryLow}},
        {Instruction::CODESIZE, {"CODESIZE", 0, 0, 1, false, Tier::Base}},
        {Instruction::CODECOPY, {"CODECOPY", 0, 3, 0, true, Tier::VeryLow}},
        {Instruction::GASPRICE, {"GASPRICE", 0, 0, 1, false, Tier::Base}},
        {Instruction::EXTCODESIZE, {"EXTCODESIZE", 0, 1, 1, false, Tier::Special}},
        {Instruction::EXTCODECOPY, {"EXTCODECOPY", 0, 4, 0, true, Tier::Special}},
        {Instruction::BLOCKHASH, {"BLOCKHASH", 0, 1, 1, false, Tier::Ext}},
        {Instruction::COINBASE, {"COINBASE", 0, 0, 1, false, Tier::Base}},
        {Instruction::TIMESTAMP, {"TIMESTAMP", 0, 0, 1, false, Tier::Base}},
        {Instruction::NUMBER, {"NUMBER", 0, 0, 1, false, Tier::Base}},
        {Instruction::DIFFICULTY, {"DIFFICULTY", 0, 0, 1, false, Tier::Base}},
        {Instruction::GASLIMIT, {"GASLIMIT", 0, 0, 1, false, Tier::Base}},
        {Instruction::JUMPTO, {"JUMPTO", 4, 1, 0, true, Tier::VeryLow}},
        {Instruction::JUMPIF, {"JUMPIF", 4, 2, 0, true, Tier::Low}},
        {Instruction::JUMPV, {"JUMPV", 4, 1, 0, true, Tier::Mid}},
        {Instruction::JUMPSUB, {"JUMPSUB", 4, 1, 0, true, Tier::Low}},
        {Instruction::JUMPSUBV, {"JUMPSUBV", 4, 1, 0, true, Tier::Mid}},
        {Instruction::RETURNSUB, {"RETURNSUB", 0, 1, 0, true, Tier::Mid}},
        {Instruction::POP, {"POP", 0, 1, 0, false, Tier::Base}},
        {Instruction::MLOAD, {"MLOAD", 0, 1, 1, false, Tier::VeryLow}},
        {Instruction::MSTORE, {"MSTORE", 0, 2, 0, true, Tier::VeryLow}},
        {Instruction::MSTORE8, {"MSTORE8", 0, 2, 0, true, Tier::VeryLow}},
        {Instruction::SLOAD, {"SLOAD", 0, 1, 1, false, Tier::Special}},
        {Instruction::SSTORE, {"SSTORE", 0, 2, 0, true, Tier::Special}},
        {Instruction::JUMP, {"JUMP", 0, 1, 0, true, Tier::Mid}},
        {Instruction::JUMPI, {"JUMPI", 0, 2, 0, true, Tier::High}},
        {Instruction::PC, {"PC", 0, 0, 1, false, Tier::Base}},
        {Instruction::MSIZE, {"MSIZE", 0, 0, 1, false, Tier::Base}},
        {Instruction::GAS, {"GAS", 0, 0, 1, false, Tier::Base}},
        {Instruction::JUMPDEST, {"JUMPDEST", 0, 0, 0, true, Tier::Special}},
        {Instruction::BEGINDATA, {"BEGINDATA", 0, 0, 0, true, Tier::Special}},
        {Instruction::BEGINSUB, {"BEGINSUB", 0, 0, 0, true, Tier::Special}},
        {Instruction::PUSH1, {"PUSH1", 1, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH2, {"PUSH2", 2, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH3, {"PUSH3", 3, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH4, {"PUSH4", 4, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH5, {"PUSH5", 5, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH6, {"PUSH6", 6, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH7, {"PUSH7", 7, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH8, {"PUSH8", 8, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH9, {"PUSH9", 9, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH10, {"PUSH10", 10, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH11, {"PUSH11", 11, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH12, {"PUSH12", 12, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH13, {"PUSH13", 13, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH14, {"PUSH14", 14, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH15, {"PUSH15", 15, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH16, {"PUSH16", 16, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH17, {"PUSH17", 17, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH18, {"PUSH18", 18, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH19, {"PUSH19", 19, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH20, {"PUSH20", 20, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH21, {"PUSH21", 21, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH22, {"PUSH22", 22, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH23, {"PUSH23", 23, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH24, {"PUSH24", 24, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH25, {"PUSH25", 25, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH26, {"PUSH26", 26, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH27, {"PUSH27", 27, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH28, {"PUSH28", 28, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH29, {"PUSH29", 29, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH30, {"PUSH30", 30, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH31, {"PUSH31", 31, 0, 1, false, Tier::VeryLow}},
        {Instruction::PUSH32, {"PUSH32", 32, 0, 1, false, Tier::VeryLow}},
        {Instruction::DUP1, {"DUP1", 0, 1, 2, false, Tier::VeryLow}},
        {Instruction::DUP2, {"DUP2", 0, 2, 3, false, Tier::VeryLow}},
        {Instruction::DUP3, {"DUP3", 0, 3, 4, false, Tier::VeryLow}},
        {Instruction::DUP4, {"DUP4", 0, 4, 5, false, Tier::VeryLow}},
        {Instruction::DUP5, {"DUP5", 0, 5, 6, false, Tier::VeryLow}},
        {Instruction::DUP6, {"DUP6", 0, 6, 7, false, Tier::VeryLow}},
        {Instruction::DUP7, {"DUP7", 0, 7, 8, false, Tier::VeryLow}},
        {Instruction::DUP8, {"DUP8", 0, 8, 9, false, Tier::VeryLow}},
        {Instruction::DUP9, {"DUP9", 0, 9, 10, false, Tier::VeryLow}},
        {Instruction::DUP10, {"DUP10", 0, 10, 11, false, Tier::VeryLow}},
        {Instruction::DUP11, {"DUP11", 0, 11, 12, false, Tier::VeryLow}},
        {Instruction::DUP12, {"DUP12", 0, 12, 13, false, Tier::VeryLow}},
        {Instruction::DUP13, {"DUP13", 0, 13, 14, false, Tier::VeryLow}},
        {Instruction::DUP14, {"DUP14", 0, 14, 15, false, Tier::VeryLow}},
        {Instruction::DUP15, {"DUP15", 0, 15, 16, false, Tier::VeryLow}},
        {Instruction::DUP16, {"DUP16", 0, 16, 17, false, Tier::VeryLow}},
        {Instruction::SWAP1, {"SWAP1", 0, 2, 2, false, Tier::VeryLow}},
        {Instruction::SWAP2, {"SWAP2", 0, 3, 3, false, Tier::VeryLow}},
        {Instruction::SWAP3, {"SWAP3", 0, 4, 4, false, Tier::VeryLow}},
        {Instruction::SWAP4, {"SWAP4", 0, 5, 5, false, Tier::VeryLow}},
        {Instruction::SWAP5, {"SWAP5", 0, 6, 6, false, Tier::VeryLow}},
        {Instruction::SWAP6, {"SWAP6", 0, 7, 7, false, Tier::VeryLow}},
        {Instruction::SWAP7, {"SWAP7", 0, 8, 8, false, Tier::VeryLow}},
        {Instruction::SWAP8, {"SWAP8", 0, 9, 9, false, Tier::VeryLow}},
        {Instruction::SWAP9, {"SWAP9", 0, 10, 10, false, Tier::VeryLow}},
        {Instruction::SWAP10, {"SWAP10", 0, 11, 11, false, Tier::VeryLow}},
        {Instruction::SWAP11, {"SWAP11", 0, 12, 12, false, Tier::VeryLow}},
        {Instruction::SWAP12, {"SWAP12", 0, 13, 13, false, Tier::VeryLow}},
        {Instruction::SWAP13, {"SWAP13", 0, 14, 14, false, Tier::VeryLow}},
        {Instruction::SWAP14, {"SWAP14", 0, 15, 15, false, Tier::VeryLow}},
        {Instruction::SWAP15, {"SWAP15", 0, 16, 16, false, Tier::VeryLow}},
        {Instruction::SWAP16, {"SWAP16", 0, 17, 17, false, Tier::VeryLow}},
        {Instruction::LOG0, {"LOG0", 0, 2, 0, true, Tier::Special}},
        {Instruction::LOG1, {"LOG1", 0, 3, 0, true, Tier::Special}},
        {Instruction::LOG2, {"LOG2", 0, 4, 0, true, Tier::Special}},
        {Instruction::LOG3, {"LOG3", 0, 5, 0, true, Tier::Special}},
        {Instruction::LOG4, {"LOG4", 0, 6, 0, true, Tier::Special}},
        {Instruction::CREATE, {"CREATE", 0, 3, 1, true, Tier::Special}},
        {Instruction::CALL, {"CALL", 0, 7, 1, true, Tier::Special}},
        {Instruction::CALLCODE, {"CALLCODE", 0, 7, 1, true, Tier::Special}},
        {Instruction::RETURN, {"RETURN", 0, 2, 0, true, Tier::Zero}},
        {Instruction::DELEGATECALL, {"DELEGATECALL", 0, 6, 1, true, Tier::Special}},
        {Instruction::SUICIDE, {"SUICIDE", 0, 1, 0, true, Tier::Special}},

        // these are generated by the interpreter - should never be in user code
        {Instruction::PUSHC, {"PUSHC", 2, 0, 1, false, Tier::VeryLow}},
        {Instruction::JUMPC, {"JUMPC", 0, 1, 0, true, Tier::Mid}},
        {Instruction::JUMPCI, {"JUMPCI", 0, 1, 0, true, Tier::High}},
        {Instruction::STOP, {"BAD", 0, 0, 0, true, Tier::Zero}},
};


// =========== ExtVMFace =========
class ExtVMFace
{
public:
    /// Null constructor.
    ExtVMFace() = default;

    /// Full constructor.
    ExtVMFace(Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256& _codeHash, unsigned _depth) : myAddress(_myAddress),
                                                                                                                                                                       caller(_caller),
                                                                                                                                                                       origin(_origin),
                                                                                                                                                                       value(_value),
                                                                                                                                                                       gasPrice(_gasPrice),
                                                                                                                                                                       data(_data),
                                                                                                                                                                       code(std::move(_code)),
                                                                                                                                                                       codeHash(_codeHash),
                                                                                                                                                                       depth(_depth)
    {
    }


    virtual ~ExtVMFace() = default;

    ExtVMFace(ExtVMFace const&) = delete;
    ExtVMFace& operator=(ExtVMFace const&) = delete;

    /// Write a value in myAddress
    virtual void setMyAddress(Address&) {}

    virtual void setOrigin(Address&) {}

    virtual void setCaller(Address&) {}

    virtual void setData(bytesConstRef&) {}

    virtual void setCode(bytes&) {}

    virtual void setCodeHash(h256&) {}

    virtual void setValue(u256&) {}


    /// Read storage location.
    virtual u256 store(u256) { return 0; }

    /// Write a value in storage.
    virtual void setStore(u256, u256) {}


    /// Read address's balance.
    virtual u256 balance(Address) { return 0; }

    /// Read address's code.

    virtual bytes const& codeAt(Address) { return NullBytes; }


    /// @returns the size of the code in bytes at the given address.
    virtual size_t codeSizeAt(Address) { return 0; }

    virtual bool exists(Address) { return false; }

    /// Suicide the associated contract and give proceeds to the given address.
    virtual void suicide(Address _myAddress) { sub.suicides.insert(_myAddress); }

    /// Create a new (contract) account.
    virtual h160 create(u256, u256&, bytesConstRef, OnOpFunc const&) { return h160(); }

    /// Make a new message call.
    virtual boost::optional<owning_bytes_ref> call(CallParameters&) = 0;

    /// Revert any changes made (by any of the other calls).
    virtual void log(h256s&& _topics, bytesConstRef _data) { sub.logs.push_back(LogEntry(myAddress, std::move(_topics), _data.toBytes())); }


    h256 blockHash(u256 _number) { return h256(); }

    /// Get the execution environment information.

    /// Return the EVM gas-price schedule for this execution context.
    virtual EVMSchedule const& evmSchedule() const { return DefaultSchedule; }

    Address author() { return Address(); }

public:
    // TODO: make private
    Address myAddress;  ///< Address associated with executing code (a contract, or contract-to-be).
    Address caller;     ///< Address which sent the message (either equal to origin or a contract).
    Address origin;     ///< Original transactor.
    u256 value;         ///< Value (in Wei) that was passed to this address.
    u256 gasPrice;      ///< Price of gas (that we already paid).
    bytesConstRef data; ///< Current input data.
    bytes code;         ///< Current code that is executing.
    h256 codeHash;      ///< SHA3 hash of the executing code
    SubState sub;       ///< Sub-band VM state (suicides, refund counter, logs).
    unsigned depth = 0; ///< Depth of the present call.
};



// =========== ExtVM =========
class ExtVM : public ExtVMFace
{
public:
    ExtVM() = delete;
    ExtVM(State& _s, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytesConstRef _code, h256& _codeHash, unsigned _depth = 0)
        : ExtVMFace(_myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(), _codeHash, _depth), m_s(_s)
    {
        // Contract: processing account must exist. In case of CALL, the ExtVM
        // is created only if an account has code (so exist). In case of CREATE
        // the account must be created first.
        assert(m_s.addressInUse(_myAddress));
    }

    virtual void setMyAddress(Address& _addr) override { myAddress = _addr; }
    virtual void setOrigin(Address& _addr) override { origin = _addr; }
    virtual void setCaller(Address& _addr) override { caller = _addr; }
    virtual void setData(bytesConstRef& _data) override { data = _data; }
    virtual void setCode(bytes& _code) override { code = _code; }
    virtual void setCodeHash(h256& _codeHash) override { codeHash = _codeHash; }
    virtual void setValue(u256& _value) override { value = _value; }

    virtual u256 store(u256 _n) override final { return m_s.storage(myAddress, _n); }
    virtual void setStore(u256 _n, u256 _v) override { m_s.setStorage(myAddress, _n, _v); }
    virtual bytes const& codeAt(Address _a) override final { return m_s.code(_a); }
    virtual size_t codeSizeAt(Address _a) override final { return m_s.codeSize(_a); }
    virtual bool exists(Address _a) override { return true; }
    virtual u256 balance(Address _a) override final { return m_s.balance(_a); }
    virtual void suicide(Address _a) override final {}; //TODO-J


    virtual h160 create(u256 _endowment, u256& io_gas, bytesConstRef _init, OnOpFunc const&) override;
    virtual boost::optional<owning_bytes_ref> call(CallParameters&) override;


    State const& state() const { return m_s; }

private:
    State& m_s; ///< A reference to the base state.
};


} // namespace sc

#endif // FABCOIN_SCVM_HPP
