
#ifndef FABCOIN_SCTRANSACTION_HPP
#define FABCOIN_SCTRANSACTION_HPP

#include "scvm.h"

namespace sc
{


// =========== TransactionBase =========

struct TransactionSkeleton {
    bool creation = false;
    Address from;
    Address to;
    u256 value;
    bytes data;
    u256 nonce = Invalid256;
    u256 gas = Invalid256;
    u256 gasPrice = Invalid256;

    std::string userReadable(bool _toProxy, std::function<std::pair<bool, std::string>(TransactionSkeleton const&)> const& _getNatSpec, std::function<std::string(Address const&)> const& _formatAddress) const;
};

/// Named-boolean type to encode whether a signature be included in the serialisation process.
enum IncludeSignature {
    WithoutSignature = 0, ///< Do not include a signature.
    WithSignature = 1,    ///< Do include a signature.
};

enum class CheckTransaction {
    None,
    Cheap,
    Everything
};

/// Encodes a transaction, ready to be exported to or freshly imported from RLP.
class TransactionBase
{
public:
    /// Constructs a null transaction.
    TransactionBase() {}

    /// Constructs a transaction from a transaction skeleton & optional secret.
    TransactionBase(TransactionSkeleton const& _ts);


    /// Constructs an unsigned message-call transaction.
    TransactionBase(bool _isCreation, u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest, bytes const& _data, u256 const& _nonce = 0) :  m_nonce(_nonce), m_value(_value), m_receiveAddress(_dest), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) {
        m_type = _isCreation ? ContractCreation : MessageCall;
    }

    /// Constructs an unsigned contract-creation transaction.
    //TransactionBase(Address const& _contractAddress, u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data, u256 const& _nonce = 0) : m_type(ContractCreation), m_nonce(_nonce), m_receiveAddress(_contractAddress), m_value(_value), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) {}

    /// Constructs a transaction from the given RLP.
    explicit TransactionBase(bytesConstRef _rlp);

    /// Constructs a transaction from the given RLP.
    explicit TransactionBase(bytes const& _rlp) : TransactionBase(&_rlp) {}

    /// Checks equality of transactions.
    bool operator==(TransactionBase const& _c) const { return m_type == _c.m_type && (m_type == ContractCreation || m_receiveAddress == _c.m_receiveAddress) && m_value == _c.m_value && m_data == _c.m_data; }
    /// Checks inequality of transactions.
    bool operator!=(TransactionBase const& _c) const { return !operator==(_c); }

    /// @returns sender of the transaction from the signature (and hash).
    Address const& sender() const;
    /// Like sender() but will never throw. @returns a null Address if the signature is invalid.
    Address const& safeSender() const noexcept;
    /// Force the sender to a particular value. This will result in an invalid transaction RLP.
    void forceSender(Address const& _a) { m_sender = _a; }

    /// @throws InvalidSValue if the signature has an invalid S value.
    void checkLowS() const;

    /// @throws InvalidSValue if the chain id is neither -4 nor equal to @a chainId
    /// Note that "-4" is the chain ID of the pre-155 rules, which should also be considered valid
    /// after EIP155
    void checkChainId(int chainId = -4) const;

    /// @returns true if transaction is non-null.
    explicit operator bool() const { return m_type != NullTransaction; }

    /// @returns true if transaction is contract-creation.
    bool isCreation() const { return m_type == ContractCreation; }

    /// Serialises this transaction to an RLPStream.
    void streamRLP(RLPStream& _s, bool _forEip155hash = false) const;

    /// @returns the RLP serialisation of this transaction.
    bytes rlp() const
    {
        RLPStream s;
        streamRLP(s);
        return s.out();
    }

    /// @returns the SHA3 hash of the RLP serialisation of this transaction.
    h256 sha3() const;

    /// @returns the amount of ETH to be transferred by this (message-call) transaction, in Wei. Synonym for endowment().
    u256 value() const { return m_value; }

    /// @returns the base fee and thus the implied exchange rate of ETH to GAS.
    u256 gasPrice() const { return m_gasPrice; }

    /// @returns the total gas to convert, paid for from sender's account. Any unused gas gets refunded once the contract is ended.
    u256 gas() const { return m_gas; }

    /// @returns the receiving address of the message-call transaction (undefined for contract-creation transactions).
    Address receiveAddress() const { return m_receiveAddress; }

    /// Synonym for receiveAddress().
    Address to() const { return m_receiveAddress; }

    /// Synonym for safeSender().
    Address from() const { return safeSender(); }

    /// @returns the data associated with this (message-call) transaction. Synonym for initCode().
    bytes const& data() const { return m_data; }

    /// @returns the transaction-count of the sender.
    u256 nonce() const { return m_nonce; }

    /// Sets the nonce to the given value. Clears any signature.
    void setNonce(u256 const& _n)
    {
        m_nonce = _n;
    }


    /// @returns amount of gas required for the basic payment.
    int64_t baseGasRequired(EVMSchedule const& _es) const { return baseGasRequired(isCreation(), &m_data, _es); }

    /// Get the fee associated for a transaction with the given data.
    static int64_t baseGasRequired(bool _contractCreation, bytesConstRef _data, EVMSchedule const& _es);

protected:
    /// Type of transaction.
    enum Type {
        NullTransaction,  ///< Null transaction.
        ContractCreation, ///< Transaction to create contracts - receiveAddress() is ignored.
        MessageCall       ///< Transaction to invoke a message call - receiveAddress() is used.
    };

    Type m_type = NullTransaction; ///< Is this a contract-creation transaction or a message-call transaction?
    u256 m_nonce;                  ///< The transaction-count of the sender.
    u256 m_value;                  ///< The amount of ETH to be transferred by this transaction. Called 'endowment' for contract-creation transactions.
    Address m_receiveAddress;      ///< The receiving address of the transaction.
    u256 m_gasPrice;               ///< The base fee and thus the implied exchange rate of ETH to GAS.
    u256 m_gas;                    ///< The total gas to convert, paid for from sender's account. Any unused gas gets refunded once the contract is ended.
    bytes m_data;                  ///< The data associated with the transaction, or the initialiser if it's a creation transaction.

    int m_chainId = -4; ///< EIP155 value for calculating transaction hash https://github.com/ethereum/EIPs/issues/155

    mutable h256 m_hashWith;  ///< Cached hash of transaction with signature.
    mutable Address m_sender; ///< Cached sender, determined from signature.
};

/// Nice name for vector of Transaction.
using TransactionBases = std::vector<TransactionBase>;

/// Simple human-readable stream-shift operator.
inline std::ostream& operator<<(std::ostream& _out, TransactionBase const& _t)
{
    _out << _t.sha3().abridged() << "{";
    if (_t.receiveAddress())
        _out << _t.receiveAddress().abridged();
    else
        _out << "[CREATE]";

    _out << "/" << _t.data().size() << "$" << _t.value() << "+" << _t.gas() << "@" << _t.gasPrice();
    _out << "<-" << _t.safeSender().abridged() << " #" << _t.nonce() << "}";
    return _out;
}

// =========== Transaction =========
enum class TransactionException {
    None = 0,
    Unknown,
    BadRLP,
    InvalidFormat,
    OutOfGasIntrinsic, ///< Too little gas to pay for the base transaction cost.
    InvalidSignature,
    InvalidNonce,
    NotEnoughCash,
    OutOfGasBase, ///< Too little gas to pay for the base transaction cost.
    BlockGasLimitReached,
    BadInstruction,
    BadJumpDestination,
    OutOfGas,   ///< Ran out of gas executing code of the transaction.
    OutOfStack, ///< Ran out of stack executing code of the transaction.
    StackUnderflow,
    CreateWithValue,
    NoInformation
};

enum class CodeDeposit {
    None = 0,
    Failed,
    Success
};

/// Base class for all exceptions.
struct Exception : virtual std::exception, virtual boost::exception {
    Exception(std::string _message = std::string()) : m_message(std::move(_message)) {}
    const char* what() const noexcept override { return m_message.empty() ? std::exception::what() : m_message.c_str(); }

private:
    std::string m_message;
};


struct VMException : virtual Exception {
};


TransactionException toTransactionException(boost::exception const& _e);
std::ostream& operator<<(std::ostream& _out, TransactionException const& _er);

/// Description of the result of executing a transaction.
struct ExecutionResult {
    u256 gasUsed = 0;
    TransactionException excepted = TransactionException::Unknown;
    Address newAddress;
    bytes output;
    CodeDeposit codeDeposit = CodeDeposit::None; ///< Failed if an attempted deposit failed due to lack of gas.
    u256 gasRefunded = 0;
    unsigned depositSize = 0; ///< Amount of code of the creation's attempted deposit.
    u256 gasForDeposit;       ///< Amount of gas remaining for the code deposit phase.
};


/// Encodes a transaction, ready to be exported to or freshly imported from RLP.
class Transaction : public TransactionBase
{
public:
    /// Constructs a null transaction.
    Transaction() {}

    /// Constructs from a transaction skeleton & optional secret.
    Transaction(TransactionSkeleton const& _ts) : TransactionBase(_ts) {}


    /// Constructs an unsigned message-call transaction.
    Transaction(bool _isCreation, u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest, bytes const& _data, u256 const& _nonce = Invalid256) : TransactionBase(_isCreation, _value, _gasPrice, _gas, _dest, _data, _nonce)
    {
    }

    /// Constructs an unsigned contract-creation transaction.
    /*Transaction(Address const& _contractAddress, u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data, u256 const& _nonce = Invalid256) : TransactionBase(_contractAddress, _value, _gasPrice, _gas, _data, _nonce)
    {
    }*/

    /// Constructs a transaction from the given RLP.
    explicit Transaction(bytesConstRef _rlp);

    /// Constructs a transaction from the given RLP.
    explicit Transaction(bytes const& _rlp) : Transaction(&_rlp) {}
};

/// Nice name for vector of Transaction.
using Transactions = std::vector<Transaction>;

class LocalisedTransaction : public Transaction
{
public:
    LocalisedTransaction(
        Transaction const& _t,
        unsigned _transactionIndex) : Transaction(_t),
                                      m_transactionIndex(_transactionIndex)
    {
    }

    unsigned transactionIndex() const { return m_transactionIndex; }

private:
    unsigned m_transactionIndex;
};

} // namespace sc

#endif // FABCOIN_SCTRANSACTION_HPP
