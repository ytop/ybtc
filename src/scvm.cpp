#include <scvm.h>
namespace sc
{


InstructionInfo instructionInfo(Instruction _inst)
{
    auto it = c_instructionInfo.find(_inst);
    if (it != c_instructionInfo.end())
        return it->second;
    return InstructionInfo({{}, 0, 0, 0, false, Tier::Invalid});
}

bool isValidInstruction(Instruction _inst)
{
    return !!c_instructionInfo.count(_inst);
}

void eachInstruction(
    bytes const& _mem,
    std::function<void(Instruction, u256 const&)> const& _onInstruction)
{
    for (auto it = _mem.begin(); it < _mem.end(); ++it) {
        Instruction instr = Instruction(*it);
        size_t additional = 0;
        if (isValidInstruction(instr))
            additional = instructionInfo(instr).additional;
        u256 data;
        for (size_t i = 0; i < additional; ++i) {
            data <<= 8;
            if (++it < _mem.end())
                data |= *it;
        }
        _onInstruction(instr, data);
    }
}

std::string disassemble(bytes const& _mem)
{
    std::stringstream ret;
    eachInstruction(_mem, [&](Instruction _instr, u256 const& _data) {
        if (!isValidInstruction(_instr))
            ret << "0x" << std::hex << int(_instr) << " ";
        else {
            InstructionInfo info = instructionInfo(_instr);
            ret << info.name << " ";
            if (info.additional)
                ret << "0x" << std::hex << _data << " ";
        }
    });
    return ret.str();
}


// ====== VMCalls  =======
//
void VM::copyDataToMemory(bytesConstRef _data, u256*& _sp)
{
    auto offset = static_cast<size_t>(*_sp--);
    s512 bigIndex = *_sp--;
    auto index = static_cast<size_t>(bigIndex);
    auto size = static_cast<size_t>(*_sp--);

    size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

    if (sizeToBeCopied > 0)
        std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
    if (size > sizeToBeCopied)
        std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}


// consolidate exception throws to avoid spraying boost code all over interpreter

void VM::throwOutOfGas()
{
    BOOST_THROW_EXCEPTION(std::range_error("Out Of Gas"));
}

void VM::throwBadInstruction()
{
    BOOST_THROW_EXCEPTION(std::range_error("Bad Instruction"));
}

void VM::throwBadJumpDestination()
{
    BOOST_THROW_EXCEPTION(std::range_error("Bad Jump Destination"));
}

void VM::throwBadStack(unsigned _size, unsigned _removed, unsigned _added)
{
    BOOST_THROW_EXCEPTION(std::range_error("Bad Stack"));
}

void VM::throwRevertInstruction(owning_bytes_ref&& _output)
{
    // We can't use BOOST_THROW_EXCEPTION here because it makes a copy of exception inside and
    // RevertInstruction has no copy constructor
    //throw RevertInstruction(std::move(_output));
    //TODO-J
    BOOST_THROW_EXCEPTION(std::range_error("Revert Instruction"));
}

int64_t VM::verifyJumpDest(u256 const& _dest, bool _throw)
{
    // check for overflow
    if (_dest <= 0x7FFFFFFFFFFFFFFF) {
        // check for within bounds and to a jump destination
        // use binary search of array because hashtable collisions are exploitable
        uint64_t pc = uint64_t(_dest);
        if (std::binary_search(m_jumpDests.begin(), m_jumpDests.end(), pc))
            return pc;
    }
    if (_throw)
        throwBadJumpDestination();
    return -1;
}


//
// interpreter cases that call out
//

void VM::caseCreate()
{
    m_bounce = &VM::interpretCases;
    m_newMemSize = memNeed(*(m_SP - 1), *(m_SP - 2));
    m_runGas = toInt63(m_schedule->createGas);
    updateMem();
    onOperation();
    updateIOGas();

    auto const& endowment = *m_SP--;
    uint64_t initOff = (uint64_t)*m_SP--;
    int64_t initSize = (uint64_t)*m_SP--;

    if (endowment) BOOST_THROW_EXCEPTION(std::range_error("Create With Value"));

    if (m_ctx->balance(m_ctx->myAddress) >= endowment && m_ctx->depth < 1024) {
        *io_gas = m_io_gas;
        u256 createGas = *io_gas;
        if (!m_schedule->staticCallDepthLimit())
            createGas -= createGas / 64;
        u256 gas = createGas;
        auto m_c = m_ctx->create(endowment, gas, bytesConstRef(m_mem.data() + initOff, initSize), m_onOp);
        *++m_SP = *((u160*)&m_c);
        *io_gas -= (createGas - gas);
        m_io_gas = uint64_t(*io_gas);
    } else
        *++m_SP = 0;
    ++m_PC;
}

void VM::caseCall()
{
    m_bounce = &VM::interpretCases;
    std::unique_ptr<CallParameters> callParams(new CallParameters());
    bytesRef output;
    if (caseCallSetup(callParams.get(), output)) {
        if (boost::optional<owning_bytes_ref> r = m_ctx->call(*callParams)) {
            r->copyTo(output);
            *++m_SP = 1;
        } else
            *++m_SP = 0;
    } else
        *++m_SP = 0;
    m_io_gas += uint64_t(callParams->gas);
    ++m_PC;
}

bool VM::caseCallSetup(CallParameters* callParams, bytesRef& o_output)
{
    m_runGas = toInt63(m_schedule->callGas);

    if (m_OP == Instruction::CALL && !m_ctx->exists(asAddress(*(m_SP - 1))))
        if (*(m_SP - 2) > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
            m_runGas += toInt63(m_schedule->callNewAccountGas);

    if (m_OP != Instruction::DELEGATECALL && *(m_SP - 2) > 0)
        m_runGas += toInt63(m_schedule->callValueTransferGas);

    size_t sizesOffset = m_OP == Instruction::DELEGATECALL ? 3 : 4;
    u256 inputOffset = m_stack[(1 + m_SP - m_stack) - sizesOffset];
    u256 inputSize = m_stack[(1 + m_SP - m_stack) - sizesOffset - 1];
    u256 outputOffset = m_stack[(1 + m_SP - m_stack) - sizesOffset - 2];
    u256 outputSize = m_stack[(1 + m_SP - m_stack) - sizesOffset - 3];
    uint64_t inputMemNeed = memNeed(inputOffset, inputSize);
    uint64_t outputMemNeed = memNeed(outputOffset, outputSize);

    m_newMemSize = std::max(inputMemNeed, outputMemNeed);
    updateMem();
    updateIOGas();

    // "Static" costs already applied. Calculate call gas.
    if (m_schedule->staticCallDepthLimit())
        // With static call depth limit we just charge the provided gas amount.
        callParams->gas = *m_SP;
    else {
        // Apply "all but one 64th" rule.
        u256 maxAllowedCallGas = m_io_gas - m_io_gas / 64;
        callParams->gas = std::min(*m_SP, maxAllowedCallGas);
    }

    m_runGas = toInt63(callParams->gas);
    onOperation();
    updateIOGas();

    if (m_OP != Instruction::DELEGATECALL && *(m_SP - 2) > 0)
        callParams->gas += m_schedule->callStipend;
    --m_SP;

    callParams->codeAddress = asAddress(*m_SP);
    --m_SP;

    if (m_OP == Instruction::DELEGATECALL) {
        callParams->apparentValue = m_ctx->value;
        callParams->valueTransfer = 0;
    } else {
        callParams->apparentValue = callParams->valueTransfer = *m_SP;
        --m_SP;
    }

    uint64_t inOff = (uint64_t)*m_SP--;
    uint64_t inSize = (uint64_t)*m_SP--;
    uint64_t outOff = (uint64_t)*m_SP--;
    uint64_t outSize = (uint64_t)*m_SP--;

    if (m_ctx->balance(m_ctx->myAddress) >= callParams->valueTransfer && m_ctx->depth < 1024) {
        callParams->onOp = m_onOp;
        callParams->senderAddress = m_OP == Instruction::DELEGATECALL ? m_ctx->caller : m_ctx->myAddress;
        callParams->receiveAddress = m_OP == Instruction::CALL ? callParams->codeAddress : m_ctx->myAddress;
        callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
        o_output = bytesRef(m_mem.data() + outOff, outSize);
        return true;
    } else
        return false;
}

// ====== VMOpt  =======

void VM::reportStackUse()
{
    static intptr_t p = 0;
    intptr_t q = intptr_t(&q);
    if (p)
        std::cerr << "STACK: " << p << " - " << q << " = " << (p - q) << std::endl;
    p = q;
}


std::array<InstructionMetric, 256> VM::c_metrics;
void VM::initMetrics()
{
    static bool done = false;
    if (!done) {
        for (unsigned i = 0; i < 256; ++i) {
            InstructionInfo op = instructionInfo((Instruction)i);
            c_metrics[i].gasPriceTier = op.gasPriceTier;
            c_metrics[i].args = op.args;
            c_metrics[i].ret = op.ret;
        }
    }
    done = true;
}

void VM::copyCode(int _ctxraBytes)
{
    // Copy code so that it can be safely modified and extend code by
    // _ctxraBytes zero bytes to allow reading virtual data at the end
    // of the code without bounds checks.
    auto extendedSize = m_ctx->code.size() + _ctxraBytes;
    m_codeSpace.reserve(extendedSize);
    m_codeSpace = m_ctx->code;
    m_codeSpace.resize(extendedSize);
    m_code = m_codeSpace.data();
}


void TRACE_OP(...){};
void TRACE_STR(...){};
void TRACE_PRE_OPT(...){};
void TRACE_POST_OPT(...){};
void TRACE_VAL(...){};
void VM::optimize()
{
    copyCode(33);

    size_t const nBytes = m_ctx->code.size();

    // build a table of jump destinations for use in verifyJumpDest

    TRACE_STR(1, "Build JUMPDEST table");
    for (size_t pc = 0; pc < nBytes; ++pc) {
        Instruction op = Instruction(m_code[pc]);
        TRACE_OP(2, pc, op);

        // make synthetic ops in user code trigger invalid instruction if run
        if (
            op == Instruction::PUSHC ||
            op == Instruction::JUMPC ||
            op == Instruction::JUMPCI) {
            TRACE_OP(1, pc, op);
            m_code[pc] = (byte)Instruction::BAD;
        }

        if (op == Instruction::JUMPDEST) {
            m_jumpDests.push_back(pc);
        } else if (
            (byte)Instruction::PUSH1 <= (byte)op &&
            (byte)op <= (byte)Instruction::PUSH32) {
            pc += (byte)op - (byte)Instruction::PUSH1 + 1;
        }
    }

    // maintain constant pool as a hash table of up to 256 u256 constants
    struct hash256 {
        // FNV chosen as good, fast, and byte-at-a-time
        const uint32_t FNV_PRIME1 = 2166136261;
        const uint32_t FNV_PRIME2 = 16777619;
        uint32_t hash = FNV_PRIME1;

        u256 (&table)[256];
        bool empty[256];

        hash256(u256 (&table)[256]) : table(table)
        {
            for (int i = 0; i < 256; ++i) {
                table[i] = 0;
                empty[i] = true;
            }
        }

        void hashInit() { hash = FNV_PRIME1; }

        // hash in successive bytes
        void hashByte(byte b) { hash ^= (b), hash *= FNV_PRIME2; }

        // fold hash into 1 byte
        byte getHash() { return ((hash >> 8) ^ hash) & 0xff; }

        // insert value at byte index in table, false if collision
        bool insertVal(byte hash, u256& val)
        {
            if (empty[hash]) {
                empty[hash] = false;
                table[hash] = val;
                return true;
            }
            return table[hash] == val;
        }
    } constantPool(m_pool);
#define CONST_POOL_HASH_INIT() constantPool.hashInit()
#define CONST_POOL_HASH_BYTE(b) constantPool.hashByte(b)
#define CONST_POOL_GET_HASH() constantPool.getHash()
#define CONST_POOL_INSERT_VAL(hash, val) constantPool.insertVal((hash), (val))

    TRACE_STR(1, "Do first pass optimizations");
    for (size_t pc = 0; pc < nBytes; ++pc) {
        u256 val = 0;
        Instruction op = Instruction(m_code[pc]);

        if ((byte)Instruction::PUSH1 <= (byte)op && (byte)op <= (byte)Instruction::PUSH32) {
            byte nPush = (byte)op - (byte)Instruction::PUSH1 + 1;


            // decode pushed bytes to integral value
            CONST_POOL_HASH_INIT();
            val = m_code[pc + 1];
            for (uint64_t i = pc + 2, n = nPush; --n; ++i) {
                val = (val << 8) | m_code[i];
                CONST_POOL_HASH_BYTE(m_code[i]);
            }

            if (1 < nPush) {
                // try to put value in constant pool at hash index
                // if there is no collision replace PUSHn with PUSHC
                TRACE_PRE_OPT(1, pc, op);
                byte hash = CONST_POOL_GET_HASH();
                if (CONST_POOL_INSERT_VAL(hash, val)) {
                    m_code[pc] = (byte)Instruction::PUSHC;
                    m_code[pc + 1] = hash;
                    m_code[pc + 2] = nPush - 1;
                    //TRACE_VAL(1, "constant pooled", val);
                }
                TRACE_POST_OPT(1, pc, op);
            }

            // replace JUMP or JUMPI to constant location with JUMPC or JUMPCI
            // verifyJumpDest is M = log(number of jump destinations)
            // outer loop is N = number of bytes in code array
            // so complexity is N log M, worst case is N log N
            size_t i = pc + nPush + 1;
            op = Instruction(m_code[i]);
            if (op == Instruction::JUMP) {
                TRACE_STR(1, "Replace const JUMPC");
                TRACE_PRE_OPT(1, i, op);

                if (0 <= verifyJumpDest(val, false))
                    m_code[i] = byte(op = Instruction::JUMPC);

                TRACE_POST_OPT(1, i, op);
            } else if (op == Instruction::JUMPI) {
                TRACE_STR(1, "Replace const JUMPCI");
                TRACE_PRE_OPT(1, i, op);

                if (0 <= verifyJumpDest(val, false))
                    m_code[i] = byte(op = Instruction::JUMPCI);

                TRACE_POST_OPT(1, i, op);
            }

            pc += nPush;
        }
    }
    TRACE_STR(1, "Finished optimizations");
}

void VM::initEntry()
{
    m_bounce = &VM::interpretCases;
    interpretCases(); // first call initializes jump table
    initMetrics();
    optimize();
}


// Implementation of EXP.
//
// This implements exponentiation by squaring algorithm.
// Is faster than boost::multiprecision::powm() because it avoids explicit
// mod operation.
// Do not inline it.
u256 VM::exp256(u256 _base, u256 _exponent)
{
    using boost::multiprecision::limb_type;
    u256 result = 1;
    while (_exponent) {
        if (static_cast<limb_type>(_exponent) & 1) // If exponent is odd.
            result *= _base;
        _base *= _base;
        _exponent >>= 1;
    }
    return result;
}

// ====== VM  =======

uint64_t VM::memNeed(u256 _offset, u256 _size)
{
    return toInt63(_size ? u512(_offset) + _size : u512(0));
}

template <class S>
S divWorkaround(S const& _a, S const& _b)
{
    return (S)(s512(_a) / s512(_b));
}

template <class S>
S modWorkaround(S const& _a, S const& _b)
{
    return (S)(s512(_a) % s512(_b));
}


//
// for decoding destinations of JUMPTO, JUMPV, JUMPSUB and JUMPSUBV
//

uint64_t VM::decodeJumpDest(const byte* const _code, uint64_t& _pc)
{
    // turn 4 MSB-first bytes in the code into a native-order integer
    uint64_t dest = _code[_pc++];
    dest = (dest << 8) | _code[_pc++];
    dest = (dest << 8) | _code[_pc++];
    dest = (dest << 8) | _code[_pc++];
    return dest;
}

uint64_t VM::decodeJumpvDest(const byte* const _code, uint64_t& _pc, u256*& _sp)
{
    // Layout of jump table in bytecode...
    //     byte opcode
    //     byte n_jumps
    //     byte table[n_jumps][4]
    //
    uint64_t i = uint64_t(*_sp--); // byte on stack indexes into jump table
    uint64_t pc = _pc;
    byte n = _code[++pc];  // byte after opcode is number of jumps
    if (i >= n) i = n - 1; // if index overflow use default jump
    pc += i * 4;           // adjust pc to index destination in table

    uint64_t dest = decodeJumpDest(_code, pc);

    _pc += 1 + n * 4; // adust input _pc to opcode after table
    return dest;
}


//
// for tracing, checking, metering, measuring ...
//
void VM::onOperation()
{
    if (m_onOp)
        (m_onOp)(++m_nSteps, m_PC, m_OP,
            m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
            m_runGas, m_io_gas, this, m_ctx);
}

void VM::checkStack(unsigned _removed, unsigned _added)
{
    int const size = 1 + m_SP - m_stack;
    int const usedSize = size - _removed;
    if (usedSize < 0 || usedSize + _added > 1024)
        throwBadStack(size, _removed, _added);
}

uint64_t VM::gasForMem(u512 _size)
{
    u512 s = _size / 32;
    return toInt63((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
}

void VM::updateIOGas()
{
    if (m_io_gas < m_runGas)
        throwOutOfGas();
    m_io_gas -= m_runGas;
}

void VM::updateGas()
{
    if (m_newMemSize > m_mem.size())
        m_runGas += toInt63(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
    m_runGas += (m_schedule->copyGas * ((m_copyMemSize + 31) / 32));
    if (m_io_gas < m_runGas)
        throwOutOfGas();
}

void VM::updateMem()
{
    m_newMemSize = (m_newMemSize + 31) / 32 * 32;
    updateGas();
    if (m_newMemSize > m_mem.size())
        m_mem.resize(m_newMemSize);
}

void VM::logGasMem()
{
    unsigned n = (unsigned)m_OP - (unsigned)Instruction::LOG0;
    m_runGas = toInt63(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * *(m_SP - 1));
    m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
    updateMem();
}

void VM::fetchInstruction()
{
    m_OP = Instruction(m_code[m_PC]);
    const InstructionMetric& metric = c_metrics[static_cast<size_t>(m_OP)];
    checkStack(metric.args, metric.ret);

    // FEES...
    m_runGas = toInt63(m_schedule->tierStepGas[static_cast<unsigned>(metric.gasPriceTier)]);
    m_newMemSize = m_mem.size();
    m_copyMemSize = 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref VM::exec(u256& _io_gas, ExtVMFace& _ctx, OnOpFunc const& _onOp)
{
    io_gas = &_io_gas;
    m_io_gas = uint64_t(_io_gas);
    m_ctx = &_ctx;
    m_schedule = &m_ctx->evmSchedule();
    m_onOp = _onOp;
    m_onFail = &VM::onOperation;

    try {
        // trampoline to minimize depth of call stack when calling out
        m_bounce = &VM::initEntry;
        do
            (this->*m_bounce)();
        while (m_bounce);

    } catch (...) {
        *io_gas = m_io_gas;
        throw;
    }

    *io_gas = m_io_gas;
    return std::move(m_output);
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
    INIT_CASES
    DO_CASES
    {
        //
        // Call-related instructions
        //

        CASE(CREATE)
        {
            m_bounce = &VM::caseCreate;
        }
        BREAK;

        CASE(DELEGATECALL)

        // Pre-homestead
        if (!m_schedule->haveDelegateCall)
            throwBadInstruction();
        CASE(STATICCALL)
        CASE(CALL)
        CASE(CALLCODE)
        {
            m_bounce = &VM::caseCall;
        }
        BREAK

        CASE(RETURN)
        {
            m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
            updateMem();
            onOperation();
            updateIOGas();

            size_t b = (size_t)*m_SP--;
            size_t s = (size_t)*m_SP--;
            m_output = owning_bytes_ref{std::move(m_mem), b, s};
            m_bounce = 0;
        }
        BREAK

        CASE(REVERT)
        {
            onOperation();
            m_copyMemSize = 0;
            updateMem();
            updateIOGas();

            size_t b = (size_t)*m_SP--;
            size_t s = (size_t)*m_SP--;
            m_output = owning_bytes_ref{std::move(m_mem), b, s};
            throwRevertInstruction(std::move(m_output));
        }
        BREAK;

        CASE(SUICIDE)
        {
            m_runGas = toInt63(m_schedule->suicideGas);
            Address dest = asAddress(*m_SP);

            // After EIP158 zero-value suicides do not have to pay account creation gas.
            if (m_ctx->balance(m_ctx->myAddress) > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
                // After EIP150 hard fork charge additional cost of sending
                // ethers to non-existing account.
                if (m_schedule->suicideChargesNewAccountGas() && !m_ctx->exists(dest))
                    m_runGas += m_schedule->callNewAccountGas;

            onOperation();
            updateIOGas();
            m_ctx->suicide(dest);
            m_bounce = 0;
        }
        BREAK

        CASE(STOP)
        {
            onOperation();
            updateIOGas();
            m_bounce = 0;
        }
        BREAK;


        //
        // instructions potentially expanding memory
        //

        CASE(MLOAD)
        {
            m_newMemSize = toInt63(*m_SP) + 32;
            updateMem();
            onOperation();
            updateIOGas();

            *m_SP = (u256) * (h256 const*)(m_mem.data() + (unsigned)*m_SP);
        }
        NEXT

            CASE(MSTORE)
        {
            m_newMemSize = toInt63(*m_SP) + 32;
            updateMem();
            onOperation();
            updateIOGas();

            *(h256*)&m_mem[(unsigned)*m_SP] = (h256) * (m_SP - 1);
            m_SP -= 2;
        }
        NEXT

            CASE(MSTORE8)
        {
            m_newMemSize = toInt63(*m_SP) + 1;
            updateMem();
            onOperation();
            updateIOGas();

            m_mem[(unsigned)*m_SP] = (byte)(*(m_SP - 1) & 0xff);
            m_SP -= 2;
        }
        NEXT

            CASE(SHA3)
        {
            m_runGas = toInt63(m_schedule->sha3Gas + (u512(*(m_SP - 1)) + 31) / 32 * m_schedule->sha3WordGas);
            m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
            updateMem();
            onOperation();
            updateIOGas();

            uint64_t inOff = (uint64_t)*m_SP--;
            uint64_t inSize = (uint64_t)*m_SP--;
            *++m_SP = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
        }
        NEXT

            CASE(LOG0)
        {
            logGasMem();
            onOperation();
            updateIOGas();

            m_ctx->log({}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t) * (m_SP - 1)));
            m_SP -= 2;
        }
        NEXT

            CASE(LOG1)
        {
            logGasMem();
            onOperation();
            updateIOGas();

            m_ctx->log({*(m_SP - 2)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t) * (m_SP - 1)));
            m_SP -= 3;
        }
        NEXT

            CASE(LOG2)
        {
            logGasMem();
            onOperation();
            updateIOGas();

            m_ctx->log({*(m_SP - 2), *(m_SP - 3)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t) * (m_SP - 1)));
            m_SP -= 4;
        }
        NEXT

            CASE(LOG3)
        {
            logGasMem();
            onOperation();
            updateIOGas();

            m_ctx->log({*(m_SP - 2), *(m_SP - 3), *(m_SP - 4)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t) * (m_SP - 1)));
            m_SP -= 5;
        }
        NEXT

            CASE(LOG4)
        {
            logGasMem();
            onOperation();
            updateIOGas();

            m_ctx->log({*(m_SP - 2), *(m_SP - 3), *(m_SP - 4), *(m_SP - 5)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t) * (m_SP - 1)));
            m_SP -= 6;
        }
        NEXT

            CASE(EXP)
        {
            u256 expon = *(m_SP - 1);
            m_runGas = toInt63(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
            onOperation();
            updateIOGas();

            u256 base = *m_SP--;
            *m_SP = exp256(base, expon);
        }
        NEXT

            //
            // ordinary instructions
            //

            CASE(ADD)
        {
            onOperation();
            updateIOGas();

            //pops two items and pushes S[-1] + S[-2] mod 2^256.
            *(m_SP - 1) += *m_SP;
            --m_SP;
        }
        NEXT

            CASE(MUL)
        {
            onOperation();
            updateIOGas();

            //pops two items and pushes S[-1] * S[-2] mod 2^256.
            *(m_SP - 1) *= *m_SP;
            --m_SP;
        }
        NEXT

            CASE(SUB)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP - *(m_SP - 1);
            --m_SP;
        }
        NEXT

            CASE(DIV)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *(m_SP - 1) ? divWorkaround(*m_SP, *(m_SP - 1)) : 0;
            --m_SP;
        }
        NEXT

            CASE(SDIV)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *(m_SP - 1) ? s2u(divWorkaround(u2s(*m_SP), u2s(*(m_SP - 1)))) : 0;
            --m_SP;
        }
        NEXT

            CASE(MOD)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *(m_SP - 1) ? modWorkaround(*m_SP, *(m_SP - 1)) : 0;
            --m_SP;
        }
        NEXT

            CASE(SMOD)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *(m_SP - 1) ? s2u(modWorkaround(u2s(*m_SP), u2s(*(m_SP - 1)))) : 0;
            --m_SP;
        }
        NEXT

            CASE(NOT)
        {
            onOperation();
            updateIOGas();

            *m_SP = ~*m_SP;
        }
        NEXT

            CASE(LT)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP < *(m_SP - 1) ? 1 : 0;
            --m_SP;
        }
        NEXT

            CASE(GT)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP > *(m_SP - 1) ? 1 : 0;
            --m_SP;
        }
        NEXT

            CASE(SLT)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = u2s(*m_SP) < u2s(*(m_SP - 1)) ? 1 : 0;
            --m_SP;
        }
        NEXT

            CASE(SGT)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = u2s(*m_SP) > u2s(*(m_SP - 1)) ? 1 : 0;
            --m_SP;
        }
        NEXT

            CASE(EQ)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP == *(m_SP - 1) ? 1 : 0;
            --m_SP;
        }
        NEXT

            CASE(ISZERO)
        {
            onOperation();
            updateIOGas();

            *m_SP = *m_SP ? 0 : 1;
        }
        NEXT

            CASE(AND)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP & *(m_SP - 1);
            --m_SP;
        }
        NEXT

            CASE(OR)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP | *(m_SP - 1);
            --m_SP;
        }
        NEXT

            CASE(XOR)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP ^ *(m_SP - 1);
            --m_SP;
        }
        NEXT

            CASE(BYTE)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 1) = *m_SP < 32 ? (*(m_SP - 1) >> (unsigned)(8 * (31 - *m_SP))) & 0xff : 0;
            --m_SP;
        }
        NEXT

            CASE(ADDMOD)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 2) = *(m_SP - 2) ? u256((u512(*m_SP) + u512(*(m_SP - 1))) % *(m_SP - 2)) : 0;
            m_SP -= 2;
        }
        NEXT

            CASE(MULMOD)
        {
            onOperation();
            updateIOGas();

            *(m_SP - 2) = *(m_SP - 2) ? u256((u512(*m_SP) * u512(*(m_SP - 1))) % *(m_SP - 2)) : 0;
            m_SP -= 2;
        }
        NEXT

            CASE(SIGNEXTEND)
        {
            onOperation();
            updateIOGas();

            if (*m_SP < 31) {
                unsigned testBit = static_cast<unsigned>(*m_SP) * 8 + 7;
                u256& number = *(m_SP - 1);
                u256 mask = ((u256(1) << testBit) - 1);
                if (boost::multiprecision::bit_test(number, testBit))
                    number |= ~mask;
                else
                    number &= mask;
            }
            --m_SP;
        }
        NEXT

            CASE(ADDRESS)
        {
            onOperation();
            updateIOGas();

            *++m_SP = fromAddress(m_ctx->myAddress);
        }
        NEXT

            CASE(ORIGIN)
        {
            onOperation();
            updateIOGas();

            *++m_SP = fromAddress(m_ctx->origin);
        }
        NEXT

            CASE(BALANCE)
        {
            m_runGas = toInt63(m_schedule->balanceGas);
            onOperation();
            updateIOGas();

            *m_SP = m_ctx->balance(asAddress(*m_SP));
        }
        NEXT


            CASE(CALLER)
        {
            onOperation();
            updateIOGas();

            *++m_SP = fromAddress(m_ctx->caller);
        }
        NEXT

            CASE(CALLVALUE)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_ctx->value;
        }
        NEXT


            CASE(CALLDATALOAD)
        {
            onOperation();
            updateIOGas();

            if (u512(*m_SP) + 31 < m_ctx->data.size())
                *m_SP = (u256) * (h256 const*)(m_ctx->data.data() + (size_t)*m_SP);
            else if (*m_SP >= m_ctx->data.size())
                *m_SP = u256(0);
            else {
                h256 r;
                for (uint64_t i = (uint64_t)*m_SP, e = (uint64_t)*m_SP + (uint64_t)32, j = 0; i < e; ++i, ++j)
                    r[j] = i < m_ctx->data.size() ? m_ctx->data[i] : 0;
                *m_SP = (u256)r;
            }
        }
        NEXT


            CASE(CALLDATASIZE)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_ctx->data.size();
        }
        NEXT

            CASE(CODESIZE)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_ctx->code.size();
        }
        NEXT

            CASE(EXTCODESIZE)
        {
            m_runGas = toInt63(m_schedule->extcodesizeGas);
            onOperation();
            updateIOGas();

            *m_SP = m_ctx->codeSizeAt(asAddress(*m_SP));
        }
        NEXT

            CASE(CALLDATACOPY)
        {
            m_copyMemSize = toInt63(*(m_SP - 2));
            m_newMemSize = memNeed(*m_SP, *(m_SP - 2));
            updateMem();
            onOperation();
            updateIOGas();

            copyDataToMemory(m_ctx->data, m_SP);
        }
        NEXT

            CASE(CODECOPY)
        {
            m_copyMemSize = toInt63(*(m_SP - 2));
            m_newMemSize = memNeed(*m_SP, *(m_SP - 2));
            updateMem();
            onOperation();
            updateIOGas();

            copyDataToMemory(&m_ctx->code, m_SP);
        }
        NEXT

            CASE(EXTCODECOPY)
        {
            m_runGas = toInt63(m_schedule->extcodecopyGas);
            m_copyMemSize = toInt63(*(m_SP - 3));
            m_newMemSize = memNeed(*(m_SP - 1), *(m_SP - 3));
            updateMem();
            onOperation();
            updateIOGas();

            Address a = asAddress(*m_SP);
            --m_SP;
            copyDataToMemory(&m_ctx->codeAt(a), m_SP);
        }
        NEXT


            CASE(GASPRICE)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_ctx->gasPrice;
        }
        NEXT

            CASE(BLOCKHASH)
        {
            onOperation();
            updateIOGas();

            *m_SP = (u256)m_ctx->blockHash(*m_SP);
        }
        NEXT

            CASE(COINBASE)
        {
            onOperation();
            updateIOGas();

            //*++m_SP = *((u160 *)&m_ctx->envInfo().author());
            auto addr = Address();
            *++m_SP = *(u160*)&addr;
        }
        NEXT

            CASE(TIMESTAMP)
        {
            onOperation();
            updateIOGas();

            *++m_SP = 123456789; // m_ctx->envInfo().timestamp();
        }
        NEXT

            CASE(NUMBER)
        {
            onOperation();
            updateIOGas();

            *++m_SP = 0; //m_ctx->envInfo().number();
        }
        NEXT

            CASE(DIFFICULTY)
        {
            onOperation();
            updateIOGas();

            *++m_SP = 0; //m_ctx->envInfo().difficulty();
        }
        NEXT

            CASE(GASLIMIT)
        {
            onOperation();
            updateIOGas();

            *++m_SP = 400000; //m_ctx->envInfo().gasLimit();
        }
        NEXT

            CASE(POP)
        {
            onOperation();
            updateIOGas();

            --m_SP;
        }
        NEXT

            CASE(PUSHC)
        {
            onOperation();
            updateIOGas();

            ++m_PC;
            *++m_SP = m_pool[m_code[m_PC]];
            ++m_PC;
            m_PC += m_code[m_PC];
        }
        CONTINUE

        CASE(PUSH1)
        {
            onOperation();
            updateIOGas();
            *++m_SP = m_code[++m_PC];
            ++m_PC;
        }
        CONTINUE

        CASE(PUSH2)
        CASE(PUSH3)
        CASE(PUSH4)
        CASE(PUSH5)
        CASE(PUSH6)
        CASE(PUSH7)
        CASE(PUSH8)
        CASE(PUSH9)
        CASE(PUSH10)
        CASE(PUSH11)
        CASE(PUSH12)
        CASE(PUSH13)
        CASE(PUSH14)
        CASE(PUSH15)
        CASE(PUSH16)
        CASE(PUSH17)
        CASE(PUSH18)
        CASE(PUSH19)
        CASE(PUSH20)
        CASE(PUSH21)
        CASE(PUSH22)
        CASE(PUSH23)
        CASE(PUSH24)
        CASE(PUSH25)
        CASE(PUSH26)
        CASE(PUSH27)
        CASE(PUSH28)
        CASE(PUSH29)
        CASE(PUSH30)
        CASE(PUSH31)
        CASE(PUSH32)
        {
            onOperation();
            updateIOGas();

            int numBytes = (int)m_OP - (int)Instruction::PUSH1 + 1;
            *++m_SP = 0;
            // Construct a number out of PUSH bytes.
            // This requires the code has been copied and extended by 32 zero
            // bytes to handle "out of code" push data here.
            for (++m_PC; numBytes--; ++m_PC)
                *m_SP = (*m_SP << 8) | m_code[m_PC];
        }
        CONTINUE

        CASE(JUMP)
        {
            onOperation();
            updateIOGas();

            m_PC = verifyJumpDest(*m_SP);
            --m_SP;
        }
        CONTINUE

        CASE(JUMPI)
        {
            onOperation();
            updateIOGas();
            if (*(m_SP - 1))
                m_PC = verifyJumpDest(*m_SP);
            else
                ++m_PC;
            m_SP -= 2;
        }
        CONTINUE

        CASE(JUMPTO)
        CASE(JUMPIF)
        CASE(JUMPV)
        CASE(JUMPSUB)
        CASE(JUMPSUBV)
        CASE(RETURNSUB)
        {
            throwBadInstruction();
        }
        CONTINUE

        CASE(JUMPC)
        {
            onOperation();
            updateIOGas();

            m_PC = uint64_t(*m_SP);
            --m_SP;
        }
        CONTINUE

        CASE(JUMPCI)
        {
            onOperation();
            updateIOGas();

            if (*(m_SP - 1))
                m_PC = uint64_t(*m_SP);
            else
                ++m_PC;
            m_SP -= 2;
        }
        CONTINUE

        CASE(DUP1)
        CASE(DUP2)
        CASE(DUP3)
        CASE(DUP4)
        CASE(DUP5)
        CASE(DUP6)
        CASE(DUP7)
        CASE(DUP8)
        CASE(DUP9)
        CASE(DUP10)
        CASE(DUP11)
        CASE(DUP12)
        CASE(DUP13)
        CASE(DUP14)
        CASE(DUP15)
        CASE(DUP16)
        {
            onOperation();
            updateIOGas();

            unsigned n = 1 + (unsigned)m_OP - (unsigned)Instruction::DUP1;
            //*(uint64_t*)(m_SP+1) = *(uint64_t*)&m_stack[(1 + m_SP - m_stack) - n];
            *(m_SP + 1) = m_stack[(1 + m_SP - m_stack) - n];
            ++m_SP;
        }
        NEXT


        CASE(SWAP1)
        CASE(SWAP2)
        CASE(SWAP3)
        CASE(SWAP4)
        CASE(SWAP5)
        CASE(SWAP6)
        CASE(SWAP7)
        CASE(SWAP8)
        CASE(SWAP9)
        CASE(SWAP10)
        CASE(SWAP11)
        CASE(SWAP12)
        CASE(SWAP13)
        CASE(SWAP14)
        CASE(SWAP15)
        CASE(SWAP16)
        {
            onOperation();
            updateIOGas();

            unsigned n = (unsigned)m_OP - (unsigned)Instruction::SWAP1 + 2;
            u256 d = *m_SP;
            *m_SP = m_stack[(1 + m_SP - m_stack) - n];
            m_stack[(1 + m_SP - m_stack) - n] = d;
        }
        NEXT


            CASE(SLOAD)
        {
            m_runGas = toInt63(m_schedule->sloadGas);
            onOperation();
            updateIOGas();

            *m_SP = m_ctx->store(*m_SP);
        }
        NEXT

            CASE(SSTORE)
        {
            if (!m_ctx->store(*m_SP) && *(m_SP - 1))
                m_runGas = toInt63(m_schedule->sstoreSetGas);
            else if (m_ctx->store(*m_SP) && !*(m_SP - 1)) {
                m_runGas = toInt63(m_schedule->sstoreResetGas);
                m_ctx->sub.refunds += m_schedule->sstoreRefundGas;
            } else
                m_runGas = toInt63(m_schedule->sstoreResetGas);
            onOperation();
            updateIOGas();

            m_ctx->setStore(*m_SP, *(m_SP - 1));
            m_SP -= 2;
        }
        NEXT

            CASE(PC)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_PC;
        }
        NEXT

            CASE(MSIZE)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_mem.size();
        }
        NEXT

            CASE(GAS)
        {
            onOperation();
            updateIOGas();

            *++m_SP = m_io_gas;
        }
        NEXT

            CASE(JUMPDEST)
        {
            m_runGas = 1;
            onOperation();
            updateIOGas();
        }
        NEXT

            CASE(BEGINSUB)
                CASE(BEGINDATA)
                    CASE(BAD)
                        DEFAULT
                        throwBadInstruction();
    }
    WHILE_CASES
}


// =========== VersionVM =========
struct VersionVM {
//this should be portable, see https://stackoverflow.com/questions/31726191/is-there-a-portable-alternative-to-c-bitfields
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t format : 2;
    uint8_t rootVM : 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t rootVM : 6;
    uint8_t format : 2;
#endif
    uint8_t vmVersion;
    uint16_t flagOptions;
    // CONSENSUS CRITICAL!
    // Do not add any other fields to this struct

    uint32_t toRaw()
    {
        return *(uint32_t*)this;
    }
    static VersionVM fromRaw(uint32_t val)
    {
        VersionVM x = *(VersionVM*)&val;
        return x;
    }
    static VersionVM GetNoExec()
    {
        VersionVM x;
        x.flagOptions = 0;
        x.rootVM = 0;
        x.format = 0;
        x.vmVersion = 0;
        return x;
    }
    static VersionVM GetEVMDefault()
    {
        VersionVM x;
        x.flagOptions = 0;
        x.rootVM = 1;
        x.format = 0;
        x.vmVersion = 0;
        return x;
    }
} __attribute__((__packed__));


// =========== ExtVM =========

static unsigned const c_depthLimit = 1024;

/// Upper bound of stack space needed by single CALL/CREATE execution. Set experimentally.
static size_t const c_singleExecutionStackSize = 14 * 1024;

/// Standard thread stack size.
static size_t const c_defaultStackSize = 8 * 1024 * 1024;


/// Stack overhead prior to allocation.
static size_t const c_entryOverhead = 128 * 1024;

/// On what depth execution should be offloaded to additional separated stack space.
static unsigned const c_offloadPoint = (c_defaultStackSize - c_entryOverhead) / c_singleExecutionStackSize;

//void goOnOffloadedStack(Executive& _e, OnOpFunc const& _onOp)
//{
//
//    boost::thread::attributes attrs;
//    attrs.set_stack_size((c_depthLimit - c_offloadPoint) * c_singleExecutionStackSize);
//
//    boost::exception_ptr exception;
//    boost::thread{attrs, [&] {
//                      try {
//                          _e.go(_onOp);
//                      } catch (...) {
//                          exception = boost::current_exception(); /
//                      }
//                  }}
//        .join();
//    if (exception)
//        boost::rethrow_exception(exception);
//}
//
//void go(unsigned _depth, Executive& _e, OnOpFunc const& _onOp)
//{
//
//
//    if (_depth == c_offloadPoint) {
//        cnote << "Stack offloading (depth: " << c_offloadPoint << ")";
//        goOnOffloadedStack(_e, _onOp);
//    } else
//        _e.go(_onOp);
//}


h160 ExtVM::create(u256 _endowment, u256& io_gas, bytesConstRef _init, OnOpFunc const&)
{
    return h160();
}

boost::optional<owning_bytes_ref> ExtVM::call(CallParameters& _p)
{
    return owning_bytes_ref{}; // Return empty output.
}

} // namespace sc
