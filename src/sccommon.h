// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Ybtc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_SCCOMMON_HPP
#define FABCOIN_SCCOMMON_HPP

#include "uint256.h"
#include "scfixedhash.h"

namespace sc
{

// misc

inline std::string toString(h256s const& _bs)
{
    std::ostringstream out;
    out << "[ ";
    for (auto i : _bs)
        out << i.abridged() << ", ";
    out << "]";
    return out.str();
}

inline bytes asBytes(std::string const& _b)
{
    return bytes((byte const*)_b.data(), (byte const*)(_b.data() + _b.size()));
}

// =========== misc =========


const unsigned c_protocolVersion = 63;
const unsigned c_minorProtocolVersion = 3;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 1;


const unsigned c_databaseVersion = c_databaseBaseVersion + (c_databaseVersionModifier << 8) + (23 << 9);

#define DEV_IGNORE_EXCEPTIONS(X) \
    try {                        \
        X;                       \
    } catch (...) {              \
    }

// ============ Misc ======
extern h256 EmptySHA3;
extern const h256 EmptyTrie;
extern const h256 c_shaNull;

inline h256 uintToh256(const uint256& in)
{
    std::vector<unsigned char> vHashBlock;
    vHashBlock.assign(in.begin(), in.end());
    return h256(vHashBlock);
}

inline uint256 h256Touint(const h256& in)
{
    std::vector<unsigned char> vHashBlock = in.asBytes();
    return uint256(vHashBlock);
}




// ====== Guard  =======

using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using SharedMutex = boost::shared_mutex;

using Guard = std::lock_guard<std::mutex>;
using UniqueGuard = std::unique_lock<std::mutex>;
using RecursiveGuard = std::lock_guard<std::recursive_mutex>;
using ReadGuard = boost::shared_lock<boost::shared_mutex>;
using UpgradableGuard = boost::upgrade_lock<boost::shared_mutex>;
using UpgradeGuard = boost::upgrade_to_unique_lock<boost::shared_mutex>;
using WriteGuard = boost::unique_lock<boost::shared_mutex>;

template <class GuardType, class MutexType>
struct GenericGuardBool : GuardType {
    GenericGuardBool(MutexType& _m) : GuardType(_m) {}
    bool b = true;
};
template <class MutexType>
struct GenericUnguardBool {
    GenericUnguardBool(MutexType& _m) : m(_m) { m.unlock(); }
    ~GenericUnguardBool() { m.lock(); }
    bool b = true;
    MutexType& m;
};
template <class MutexType>
struct GenericUnguardSharedBool {
    GenericUnguardSharedBool(MutexType& _m) : m(_m) { m.unlock_shared(); }
    ~GenericUnguardSharedBool() { m.lock_shared(); }
    bool b = true;
    MutexType& m;
};

/** @brief Simple lock that waits for release without making context switch */
class SpinLock
{
public:
    SpinLock() { m_lock.clear(); }
    void lock()
    {
        while (m_lock.test_and_set(std::memory_order_acquire)) {
        }
    }
    void unlock() { m_lock.clear(std::memory_order_release); }

private:
    std::atomic_flag m_lock;
};
using SpinGuard = std::lock_guard<SpinLock>;

template <class N>
class Notified
{
public:
    Notified() {}
    Notified(N const& _v) : m_value(_v) {}
    Notified(Notified const&) = delete;
    Notified& operator=(N const& _v)
    {
        UniqueGuard l(m_mutex);
        m_value = _v;
        m_cv.notify_all();
        return *this;
    }

    operator N() const
    {
        UniqueGuard l(m_mutex);
        return m_value;
    }

    void wait() const
    {
        N old;
        {
            UniqueGuard l(m_mutex);
            old = m_value;
        }
        waitNot(old);
    }
    void wait(N const& _v) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait(l, [&]() { return m_value == _v; });
    }
    void waitNot(N const& _v) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait(l, [&]() { return m_value != _v; });
    }
    template <class F>
    void wait(F const& _f) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait(l, _f);
    }

    template <class R, class P>
    void wait(std::chrono::duration<R, P> _d) const
    {
        N old;
        {
            UniqueGuard l(m_mutex);
            old = m_value;
        }
        waitNot(_d, old);
    }
    template <class R, class P>
    void wait(std::chrono::duration<R, P> _d, N const& _v) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait_for(l, _d, [&]() { return m_value == _v; });
    }
    template <class R, class P>
    void waitNot(std::chrono::duration<R, P> _d, N const& _v) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait_for(l, _d, [&]() { return m_value != _v; });
    }
    template <class R, class P, class F>
    void wait(std::chrono::duration<R, P> _d, F const& _f) const
    {
        UniqueGuard l(m_mutex);
        m_cv.wait_for(l, _d, _f);
    }

private:
    mutable Mutex m_mutex;
    mutable std::condition_variable m_cv;
    N m_value;
};


#define DEV_GUARDED(MUTEX) \
    for (GenericGuardBool<Guard, Mutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_READ_GUARDED(MUTEX) \
    for (GenericGuardBool<ReadGuard, SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_WRITE_GUARDED(MUTEX) \
    for (GenericGuardBool<WriteGuard, SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_RECURSIVE_GUARDED(MUTEX) \
    for (GenericGuardBool<RecursiveGuard, RecursiveMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_UNGUARDED(MUTEX) \
    for (GenericUnguardBool<Mutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_READ_UNGUARDED(MUTEX) \
    for (GenericUnguardSharedBool<SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)
#define DEV_WRITE_UNGUARDED(MUTEX) \
    for (GenericUnguardBool<SharedMutex> __eth_l(MUTEX); __eth_l.b; __eth_l.b = false)


// =========== CodeSizeCache =========
/**
 * @brief Simple thread-safe cache to store a mapping from code hash to code size.
 * If the cache is full, a random element is removed.
 */
class CodeSizeCache
{
public:
    void store(h256 const& _hash, size_t size)
    {
        UniqueGuard g(x_cache);
        if (m_cache.size() >= c_maxSize)
            removeRandomElement();
        m_cache[_hash] = size;
    }
    bool contains(h256 const& _hash) const
    {
        UniqueGuard g(x_cache);
        return m_cache.count(_hash);
    }
    size_t get(h256 const& _hash) const
    {
        UniqueGuard g(x_cache);
        return m_cache.at(_hash);
    }

    static CodeSizeCache& instance()
    {
        static CodeSizeCache cache;
        return cache;
    }

private:
    /// Removes a random element from the cache.
    void removeRandomElement()
    {
        if (!m_cache.empty()) {
            auto it = m_cache.lower_bound(h256::random());
            if (it == m_cache.end())
                it = m_cache.begin();
            m_cache.erase(it);
        }
    }

    static const size_t c_maxSize = 50000;
    mutable Mutex x_cache;
    std::map<h256, size_t> m_cache;
};
/// Interprets @a _u as a two's complement signed number and returns the resulting s256.
inline s256 u2s(u256 _u)
{
    static const bigint c_end = bigint(1) << 256;
    if (boost::multiprecision::bit_test(_u, 255))
        return s256(-(c_end - _u));
    else
        return s256(_u);
}

/// @returns the two's complement signed representation of the signed number _u.
inline u256 s2u(s256 _u)
{
    static const bigint c_end = bigint(1) << 256;
    if (_u >= 0)
        return u256(_u);
    else
        return u256(c_end + _u);
}

};

#endif // FABCOIN_SCCOMMON_HPP
