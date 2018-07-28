#include <scexecutive.h>
namespace sc
{


// =========== Executive =========
const char* VMTraceChannel::name() { return "EVM"; }
const char* ExecutiveWarnChannel::name() { return WarnChannel::name(); }


StandardTrace::StandardTrace()
{
}


bool changesMemory(Instruction _inst)
{
    return _inst == Instruction::MSTORE ||
           _inst == Instruction::MSTORE8 ||
           _inst == Instruction::MLOAD ||
           _inst == Instruction::CREATE ||
           _inst == Instruction::CALL ||
           _inst == Instruction::CALLCODE ||
           _inst == Instruction::SHA3 ||
           _inst == Instruction::CALLDATACOPY ||
           _inst == Instruction::CODECOPY ||
           _inst == Instruction::EXTCODECOPY ||
           _inst == Instruction::DELEGATECALL;
}

bool changesStorage(Instruction _inst)
{
    return _inst == Instruction::SSTORE;
}

void StandardTrace::operator()(uint64_t _steps, uint64_t PC, Instruction inst, bigint newMemSize, bigint gasCost, bigint gas, VM* voidVM, ExtVMFace const* voidExt)
{
    return;
}

std::string StandardTrace::json(bool _styled) const
{
    return "";
}

Executive::Executive(State& _s, unsigned _txIndex, unsigned _level) : m_s(_s), 
                                                                      m_depth(_level)
{
}

u256 Executive::gasUsed() const
{
    return m_t.gas() - m_gas;
}

void Executive::accrueSubState(SubState& _parentContext)
{
    if (m_ext)
        _parentContext += m_ext->sub;
}

void Executive::initialize(Transaction const& _transaction)
{
    m_t = _transaction;


    //No base gas checked
    m_baseGasRequired = 0;
    if (m_baseGasRequired > m_t.gas()) {
        clog(ExecutiveWarnChannel) << "Not enough gas to pay for the transaction: Require >" << m_baseGasRequired << " Got" << m_t.gas();
        m_excepted = TransactionException::OutOfGasBase;
        BOOST_THROW_EXCEPTION(std::range_error("Out Of Gas Base"));
    }

    // Avoid invalid transactions.
    u256 nonceReq;
    try {
        nonceReq = m_s.getNonce(m_t.sender());
    } catch (...) {
        clog(ExecutiveWarnChannel) << "Invalid Signature";
        m_excepted = TransactionException::InvalidSignature;
        throw;
    }
    /*if (m_t.nonce() != nonceReq) {
        clog(ExecutiveWarnChannel) << "Invalid Nonce: Require" << nonceReq << " Got" << m_t.nonce();
        m_excepted = TransactionException::InvalidNonce;
        BOOST_THROW_EXCEPTION(std::range_error("Invalid Nonce"));
    }*/

    // Avoid unaffordable transactions.
    bigint gasCost = (bigint)m_t.gas() * m_t.gasPrice();
    bigint totalCost = m_t.value() + gasCost;
    /*if (m_s.balance(m_t.sender()) < totalCost) {
        clog(ExecutiveWarnChannel) << "Not enough cash: Require >" << totalCost << "=" << m_t.gas() << "*" << m_t.gasPrice() << "+" << m_t.value() << " Got" << m_s.balance(m_t.sender()) << "for sender: " << m_t.sender();
        m_excepted = TransactionException::NotEnoughCash;
        BOOST_THROW_EXCEPTION(std::range_error("Not Enough Cash"));
    }*/
    m_gasCost = (u256)gasCost; // Convert back to 256-bit, safe now.
}

bool Executive::execute()
{
    // Entry point for a user-executed transaction.

    // Pay...
    //clog(StateDetail) << "Paying" << formatBalance(m_gasCost) << "from sender for gas (" << m_t.gas() << "gas at" << formatBalance(m_t.gasPrice()) << ")";
    //m_s.subBalance(m_t.sender(), m_gasCost);

    if (m_t.isCreation())
        return create(m_t.sender(), m_t.value(), m_t.gasPrice(), m_t.gas() - (u256)m_baseGasRequired, &m_t.data(), m_t.sender(), m_t.receiveAddress());
    else
        return call(m_t.receiveAddress(), m_t.sender(), m_t.value(), m_t.gasPrice(), bytesConstRef(&m_t.data()), m_t.gas() - (u256)m_baseGasRequired);
}

bool Executive::call(Address _receiveAddress, Address _senderAddress, u256 _value, u256 _gasPrice, bytesConstRef _data, u256 _gas)
{
    CallParameters params{_senderAddress, _receiveAddress, _receiveAddress, _value, _value, _gas, _data, {}};
    return call(params, _gasPrice, _senderAddress);
}

bool Executive::call(CallParameters const& _p, u256 const& _gasPrice, Address const& _origin)
{
    // If external transaction.
    if (m_t) {
        // FIXME: changelog contains unrevertable balance change that paid
        //        for the transaction.
        // Increment associated nonce for sender.
        m_s.incNonce(_p.senderAddress);
    }

    m_savepoint = m_s.savepoint();
    m_gas = _p.gas;
    if (m_s.addressHasCode(_p.codeAddress)) {
        bytes const& c = m_s.code(_p.codeAddress);
        h256 codeHash = m_s.codeHash(_p.codeAddress);
        m_ext = std::make_shared<ExtVM>(m_s, _p.receiveAddress, _p.senderAddress, _origin, _p.apparentValue, _gasPrice, _p.data, &c, codeHash, m_depth);
    }

    // Transfer ether.
    m_s.transferBalance(_p.senderAddress, _p.receiveAddress, _p.valueTransfer);
    return !m_ext;
}

bool Executive::create(Address _sender, u256 _endowment, u256 _gasPrice, u256 _gas, bytesConstRef _init, Address _origin, Address _myAddress)
{
    //u256 nonce = m_s.getNonce(_sender);
    m_s.incNonce(_sender);

    m_savepoint = m_s.savepoint();

    m_isCreation = true;

    // We can allow for the reverted state (i.e. that with which m_ext is constructed) to contain the m_orig.address, since
    // we delete it explicitly if we decide we need to revert.
    //m_newAddress = right160(sha3(rlpList(_sender, nonce)));
    m_newAddress = _myAddress;
    m_gas = _gas;

    // Transfer ether before deploying the code. This will also create new
    // account if it does not exist yet.
    m_s.transferBalance(_sender, m_newAddress, _endowment);

    // Schedule _init execution if not empty.
    if (!_init.empty()) {
        h256 h = sha3(_init);
        m_ext = std::make_shared<ExtVM>(m_s, m_newAddress, _sender, _origin, _endowment, _gasPrice, bytesConstRef(), _init, h, m_depth);
    } else if (m_s.addressHasCode(m_newAddress))
        // Overwrite with empty code in case the account already has a code
        // (address collision -- not real life case but we can check it with
        // synthetic tests).
        m_s.setNewCode(m_newAddress, {});

    return !m_ext;
}

OnOpFunc Executive::simpleTrace()
{
    return [](uint64_t steps, uint64_t PC, Instruction inst, bigint newMemSize, bigint gasCost, bigint gas, VM* voidVM, ExtVMFace const* voidExt) {
    };
}

bool Executive::go(OnOpFunc const& _onOp)
{
    if (m_ext) {
        try {
            // Create VM instance. Force Interpreter if tracing requested.
            auto vm = std::unique_ptr<VMFace>(new VM);
            if (m_isCreation) {
                auto out = vm->exec(m_gas, *m_ext, _onOp);
                if (m_res) {
                    m_res->gasForDeposit = m_gas;
                    m_res->depositSize = out.size();
                }
                if (out.size() > m_ext->evmSchedule().maxCodeSize)
                    BOOST_THROW_EXCEPTION(std::range_error("Out Of Gas"));
                else if (out.size() * m_ext->evmSchedule().createDataGas <= m_gas) {
                    if (m_res)
                        m_res->codeDeposit = CodeDeposit::Success;
                    m_gas -= out.size() * m_ext->evmSchedule().createDataGas;
                } else {
                    if (m_ext->evmSchedule().exceptionalFailedCodeDeposit)
                        BOOST_THROW_EXCEPTION(std::range_error("Out Of Gas"));
                    else {
                        if (m_res)
                            m_res->codeDeposit = CodeDeposit::Failed;
                        out = {};
                    }
                }
                if (m_res)
                    m_res->output = out.toVector(); // copy output to execution result
                m_s.setNewCode(m_ext->myAddress, out.toVector());
            } else {
                m_output = vm->exec(m_gas, *m_ext, _onOp);
                if (m_res)
                    // Copy full output:
                    m_res->output = m_output.toVector();
            }
        } catch (VMException const& _e) {
            clog(StateSafeExceptions) << "Safe VM Exception. ";
            m_gas = 0;
            m_excepted = toTransactionException(_e);
            revert();
        } catch (boost::exception const& _e) {
            // TODO: AUDIT: check that this can never reasonably happen. Consider what to do if it does.
            cwarn << "Unexpected exception in VM. There may be a bug in this implementation. ";
            exit(1);
            // Another solution would be to reject this transaction, but that also
            // has drawbacks. Essentially, the amount of ram has to be increased here.
        } catch (std::exception const& _e) {
            // TODO: AUDIT: check that this can never reasonably happen. Consider what to do if it does.
            cwarn << "Unexpected std::exception in VM. Not enough RAM? " << _e.what();
            exit(1);
            // Another solution would be to reject this transaction, but that also
            // has drawbacks. Essentially, the amount of ram has to be increased here.
        }
    }
    return true;
}

void Executive::finalize()
{
    // Accumulate refunds for suicides.
    if (m_ext)
        m_ext->sub.refunds += m_ext->evmSchedule().suicideRefundGas * m_ext->sub.suicides.size();

    // SSTORE refunds...
    // must be done before the miner gets the fees.
    auto a = (m_t.gas() - m_gas) / 2 < m_ext->sub.refunds ? (m_t.gas() - m_gas) / 2 : m_ext->sub.refunds;
    m_refunded = m_ext ? a : 0;
    m_gas += m_refunded;

    if (m_t) {
        m_s.addBalance(m_t.sender(), m_gas * m_t.gasPrice());
    }

    // Suicides...
    if (m_ext)
        for (auto a : m_ext->sub.suicides)
            m_s.kill(a);

    // Logs..
    if (m_ext)
        m_logs = m_ext->sub.logs;

    if (m_res) // Collect results
    {
        m_res->gasUsed = gasUsed();
        m_res->excepted = m_excepted; // TODO: m_except is used only in ExtVM::call
        m_res->newAddress = m_newAddress;
        m_res->gasRefunded = m_ext ? m_ext->sub.refunds : 0;
    }
}

void Executive::revert()
{
    if (m_ext)
        m_ext->sub.clear();

    // Set result address to the null one.
    m_newAddress = {};
    m_s.rollback(m_savepoint);
}

} // namespace sc
