#include <sctransaction.h>
namespace sc
{



// =========== TransactionBase =========
TransactionBase::TransactionBase(TransactionSkeleton const& _ts) : m_type(_ts.creation ? ContractCreation : MessageCall),
                                                                   m_nonce(_ts.nonce),
                                                                   m_value(_ts.value),
                                                                   m_receiveAddress(_ts.to),
                                                                   m_gasPrice(_ts.gasPrice),
                                                                   m_gas(_ts.gas),
                                                                   m_data(_ts.data),
                                                                   m_sender(_ts.from)
{
}

TransactionBase::TransactionBase(bytesConstRef _rlpData)
{
    int field = 0;
    RLP rlp(_rlpData);
    try {
        if (!rlp.isList())
            BOOST_THROW_EXCEPTION(std::range_error("transaction RLP must be a list"));

        m_nonce = rlp[field = 0].toInt<u256>();
        m_gasPrice = rlp[field = 1].toInt<u256>();
        m_gas = rlp[field = 2].toInt<u256>();
        m_type = rlp[field = 3].isEmpty() ? ContractCreation : MessageCall;
        m_receiveAddress = rlp[field = 3].isEmpty() ? Address() : rlp[field = 3].toHash<Address>(RLP::VeryStrict);
        m_value = rlp[field = 4].toInt<u256>();

        if (!rlp[field = 5].isData())
            BOOST_THROW_EXCEPTION(std::range_error("transaction data RLP must be an array"));

        m_data = rlp[field = 5].toBytes();

        byte v = rlp[field = 6].toInt<byte>();

        if (v > 36)
            m_chainId = (v - 35) / 2;
        else if (v == 27 || v == 28)
            m_chainId = -4;

        v = v - (m_chainId * 2 + 35);

        if (rlp.itemCount() > 9)
            BOOST_THROW_EXCEPTION(std::range_error("to many fields in the transaction RLP"));


    } catch (boost::exception& _e) {
        BOOST_THROW_EXCEPTION(std::range_error("invalid transaction format"));
    }
}
Address ZeroAddress = Address();
Address const& TransactionBase::safeSender() const noexcept
{
    try {
        return sender();
    } catch (...) {
        return ZeroAddress;
    }
}

Address const& TransactionBase::sender() const
{
    return m_sender;
}


void TransactionBase::streamRLP(RLPStream& _s, bool _forEip155hash) const
{
    if (m_type == NullTransaction)
        return;

    _s.appendList((_forEip155hash ? 3 : 0) + 6);
    _s << m_nonce << m_gasPrice << m_gas;
    if (m_type == MessageCall)
        _s << m_receiveAddress;
    else
        _s << "";
    _s << m_value << m_data;

    if (_forEip155hash)
        _s << m_chainId << 0 << 0;
}

static const u256 c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");

void TransactionBase::checkLowS() const
{
}

void TransactionBase::checkChainId(int chainId) const
{
}

int64_t TransactionBase::baseGasRequired(bool _contractCreation, bytesConstRef _data, EVMSchedule const& _es)
{
    int64_t g = _contractCreation ? _es.txCreateGas : _es.txGas;

    // Calculate the cost of input data.
    // No risk of overflow by using int64 until txDataNonZeroGas is quite small
    // (the value not in billions).
    for (auto i : _data)
        g += i ? _es.txDataNonZeroGas : _es.txDataZeroGas;
    return g;
}

h256 TransactionBase::sha3() const
{
    auto ret = sha3();

    return ret;
}


// =========== Transaction =========

#define ETH_ADDRESS_DEBUG 0

std::ostream& operator<<(std::ostream& _out, ExecutionResult const& _er)
{
    _out << "{" << _er.gasUsed << ", " << _er.newAddress << ", " << toHex(_er.output) << "}";
    return _out;
}

TransactionException toTransactionException(boost::exception const& _e)
{
    return TransactionException::Unknown;
}

std::ostream& operator<<(std::ostream& _out, TransactionException const& _er)
{
    switch (_er) {
    case TransactionException::None:
        _out << "None";
        break;
    case TransactionException::BadRLP:
        _out << "BadRLP";
        break;
    case TransactionException::InvalidFormat:
        _out << "InvalidFormat";
        break;
    case TransactionException::OutOfGasIntrinsic:
        _out << "OutOfGasIntrinsic";
        break;
    case TransactionException::InvalidSignature:
        _out << "InvalidSignature";
        break;
    case TransactionException::InvalidNonce:
        _out << "InvalidNonce";
        break;
    case TransactionException::NotEnoughCash:
        _out << "NotEnoughCash";
        break;
    case TransactionException::OutOfGasBase:
        _out << "OutOfGasBase";
        break;
    case TransactionException::BlockGasLimitReached:
        _out << "BlockGasLimitReached";
        break;
    case TransactionException::BadInstruction:
        _out << "BadInstruction";
        break;
    case TransactionException::BadJumpDestination:
        _out << "BadJumpDestination";
        break;
    case TransactionException::OutOfGas:
        _out << "OutOfGas";
        break;
    case TransactionException::OutOfStack:
        _out << "OutOfStack";
        break;
    case TransactionException::StackUnderflow:
        _out << "StackUnderflow";
        break;
    case TransactionException::CreateWithValue:
        _out << "CreateWithValue";
        break;
    case TransactionException::NoInformation:
        _out << "NoInformation";
        break;
    default:
        _out << "Unknown";
        break;
    }
    return _out;
}

Transaction::Transaction(bytesConstRef _rlpData) : TransactionBase(_rlpData)
{
}


} // namespace sc
