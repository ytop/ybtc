#include <scdb.h>
namespace sc
{


// ====== FileSystem  =======
static std::string s_ethereumDatadir;
static std::string s_ethereumIpcPath;

void setDataDir(std::string const& _dataDir)
{
    s_ethereumDatadir = _dataDir;
}

void setIpcPath(std::string const& _ipcDir)
{
    s_ethereumIpcPath = _ipcDir;
}

std::string getIpcPath()
{
    if (s_ethereumIpcPath.empty())
        return std::string(getDataDir());
    else {
        size_t socketPos = s_ethereumIpcPath.rfind("geth.ipc");
        if (socketPos != std::string::npos)
            return s_ethereumIpcPath.substr(0, socketPos);
        return s_ethereumIpcPath;
    }
}

std::string getDataDir(std::string _prefix)
{
    if (_prefix.empty())
        _prefix = "ethereum";
    if (_prefix == "ethereum" && !s_ethereumDatadir.empty())
        return s_ethereumDatadir;
    return getDefaultDataDir(_prefix);
}

std::string getDefaultDataDir(std::string _prefix)
{
    if (_prefix.empty())
        _prefix = "ethereum";

    boost::filesystem::path dataDirPath;
    char const* homeDir = getenv("HOME");
    if (!homeDir || strlen(homeDir) == 0) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd)
            homeDir = pwd->pw_dir;
    }

    if (!homeDir || strlen(homeDir) == 0)
        dataDirPath = boost::filesystem::path("/");
    else
        dataDirPath = boost::filesystem::path(homeDir);

    return (dataDirPath / ("." + _prefix)).string();
}

// ====== Log  =======
// Logging
int g_logVerbosity = 5;
std::mutex x_logOverride;

/// Map of Log Channel types to bool, false forces the channel to be disabled, true forces it to be enabled.
/// If a channel has no entry, then it will output as long as its verbosity (LogChannel::verbosity) is less than
/// or equal to the currently output verbosity (g_logVerbosity).
static std::map<std::type_info const*, bool> s_logOverride;

bool isChannelVisible(std::type_info const* _ch, bool _default)
{
    Guard l(x_logOverride);
    if (s_logOverride.count(_ch))
        return s_logOverride[_ch];
    return _default;
}

LogOverrideAux::LogOverrideAux(std::type_info const* _ch, bool _value) : m_ch(_ch)
{
    Guard l(x_logOverride);
    m_old = s_logOverride.count(_ch) ? (int)s_logOverride[_ch] : c_null;
    s_logOverride[m_ch] = _value;
}

LogOverrideAux::~LogOverrideAux()
{
    Guard l(x_logOverride);
    if (m_old == c_null)
        s_logOverride.erase(m_ch);
    else
        s_logOverride[m_ch] = (bool)m_old;
}


const char* LogChannel::name()
{
    return EthGray "...";
}
const char* LeftChannel::name() { return EthNavy "<--"; }
const char* RightChannel::name() { return EthGreen "-->"; }
const char* WarnChannel::name() { return EthOnRed EthBlackBold "  X"; }
const char* NoteChannel::name() { return EthBlue "  i"; }
const char* DebugChannel::name() { return EthWhite "  D"; }
const char* TraceChannel::name() { return EthGray "..."; }


LogOutputStreamBase::LogOutputStreamBase(char const* _id, std::type_info const* _info, unsigned _v, bool _autospacing) : m_autospacing(_autospacing),
                                                                                                                         m_verbosity(_v)
{
    Guard l(x_logOverride);
    auto it = s_logOverride.find(_info);
    if ((it != s_logOverride.end() && it->second == true) || (it == s_logOverride.end() && (int)_v <= g_logVerbosity)) {
        time_t rawTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        unsigned ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
        char buf[24];
        if (strftime(buf, 24, "%X", localtime(&rawTime)) == 0)
            buf[0] = '\0'; // empty if case strftime fails
        static char const* c_begin = "  " EthViolet;
        static char const* c_sep1 = EthReset EthBlack "|" EthNavy;
        static char const* c_sep2 = EthReset EthBlack "|" EthTeal;
        static char const* c_end = EthReset "  ";
        m_sstr << _id << c_begin << buf << "." << std::setw(3) << std::setfill('0') << ms;
        m_sstr << c_sep1 << getThreadName() << ThreadContext::join(c_sep2) << c_end;
    }
}

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogName {
    ThreadLocalLogName(std::string const& _name) { m_name.reset(new std::string(_name)); }
    boost::thread_specific_ptr<std::string> m_name;
};

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogContext {
    ThreadLocalLogContext() = default;

    void push(std::string const& _name)
    {
        if (!m_contexts.get())
            m_contexts.reset(new std::vector<std::string>);
        m_contexts->push_back(_name);
    }

    void pop()
    {
        m_contexts->pop_back();
    }

    std::string join(std::string const& _prior)
    {
        std::string ret;
        if (m_contexts.get())
            for (auto const& i : *m_contexts)
                ret += _prior + i;
        return ret;
    }

    boost::thread_specific_ptr<std::vector<std::string>> m_contexts;
};

ThreadLocalLogContext g_logThreadContext;

ThreadLocalLogName g_logThreadName("main");

void ThreadContext::push(std::string const& _n)
{
    g_logThreadContext.push(_n);
}

void ThreadContext::pop()
{
    g_logThreadContext.pop();
}

std::string ThreadContext::join(std::string const& _prior)
{
    return g_logThreadContext.join(_prior);
}

// foward declare without all of Windows.h
#if defined(_WIN32)
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
#endif

std::string getThreadName()
{
#ifdef FASC_BUILD
    return "";
#else
#if defined(__GLIBC__) || defined(__APPLE__)
    char buffer[128];
    pthread_getname_np(pthread_self(), buffer, 127);
    buffer[127] = 0;
    return buffer;
#else
    return g_logThreadName.m_name.get() ? *g_logThreadName.m_name.get() : "<unknown>";
#endif
#endif
}

void setThreadName(std::string const& _n)
{
#ifndef FASC_BUILD
#if defined(__GLIBC__)
    pthread_setname_np(pthread_self(), _n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(_n.c_str());
#else
    g_logThreadName.m_name.reset(new std::string(_n));
#endif
#endif
}

void simpleDebugOut(std::string const& _s, char const*)
{
    static SpinLock s_lock;
    SpinGuard l(s_lock);

    std::cerr << _s << std::endl
              << std::flush;
}

std::function<void(std::string const&, char const*)> g_logPost = simpleDebugOut;


// ====== MemoryDB  =======
const char* DBChannel::name() { return "TDB"; }
const char* DBWarn::name() { return "TDB"; }

std::unordered_map<h256, std::string> MemoryDB::get() const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    std::unordered_map<h256, std::string> ret;
    for (auto const& i : m_main)
        if (!m_enforceRefs || i.second.second > 0)
            ret.insert(make_pair(i.first, i.second.first));
    return ret;
}

MemoryDB& MemoryDB::operator=(MemoryDB const& _c)
{
    if (this == &_c)
        return *this;
#if DEV_GUARDED_DB
    ReadGuard l(_c.x_this);
    WriteGuard l2(x_this);
#endif
    m_main = _c.m_main;
    m_aux = _c.m_aux;
    return *this;
}

std::string MemoryDB::lookup(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end()) {
        if (!m_enforceRefs || it->second.second > 0)
            return it->second.first;
        else
            cwarn << "Lookup required for value with refcount == 0. This is probably a critical trie issue" << _h;
    }
    return std::string();
}

bool MemoryDB::exists(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end() && (!m_enforceRefs || it->second.second > 0))
        return true;
    return false;
}

void MemoryDB::insert(h256 const& _h, bytesConstRef _v)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    auto it = m_main.find(_h);
    if (it != m_main.end()) {
        it->second.first = _v.toString();
        it->second.second++;
    } else
        m_main[_h] = make_pair(_v.toString(), 1);
#if ETH_PARANOIA
    dbdebug << "INST" << _h << "=>" << m_main[_h].second;
#endif
}

bool MemoryDB::kill(h256 const& _h)
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    if (m_main.count(_h)) {
        if (m_main[_h].second > 0) {
            m_main[_h].second--;
            return true;
        }
#if ETH_PARANOIA
        else {
            // If we get to this point, then there was probably a node in the level DB which we need to remove and which we have previously
            // used as part of the memory-based MemoryDB. Nothing to be worried about *as long as the node exists in the DB*.
            dbdebug << "NOKILL-WAS" << _h;
        }
        dbdebug << "KILL" << _h << "=>" << m_main[_h].second;
    } else {
        dbdebug << "NOKILL" << _h;
#endif
    }
    return false;
}

bytes MemoryDB::lookupAux(h256 const& _h) const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    auto it = m_aux.find(_h);
    if (it != m_aux.end() && (!m_enforceRefs || it->second.second))
        return it->second.first;
    return bytes();
}

void MemoryDB::removeAux(h256 const& _h)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_aux[_h].second = false;
}

void MemoryDB::insertAux(h256 const& _h, bytesConstRef _v)
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_aux[_h] = make_pair(_v.toBytes(), true);
}

void MemoryDB::purge()
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    // purge m_main
    for (auto it = m_main.begin(); it != m_main.end();)
        if (it->second.second)
            ++it;
        else
            it = m_main.erase(it);

    // purge m_aux
    for (auto it = m_aux.begin(); it != m_aux.end();)
        if (it->second.second)
            ++it;
        else
            it = m_aux.erase(it);
}

h256Hash MemoryDB::keys() const
{
#if DEV_GUARDED_DB
    ReadGuard l(x_this);
#endif
    h256Hash ret;
    for (auto const& i : m_main)
        if (i.second.second)
            ret.insert(i.first);
    return ret;
}

// ====== OverlayDB  =======


OverlayDB::~OverlayDB()
{
    if (m_db.use_count() == 1 && m_db.get())
        ctrace << "Closing state DB";
}

class WriteBatchNoter : public ldb::WriteBatch::Handler
{
    virtual void Put(ldb::Slice const& _key, ldb::Slice const& _value) { cnote << "Put" << toHex(bytesConstRef(_key)) << "=>" << toHex(bytesConstRef(_value)); }
    virtual void Delete(ldb::Slice const& _key) {  cnote << "Delete" << toHex(bytesConstRef(_key));}
};

void OverlayDB::commit()
{
    if (m_db) {
        ldb::WriteBatch batch;
        //		cnote << "Committing nodes to disk DB:";
#if DEV_GUARDED_DB
        DEV_READ_GUARDED(x_this)
#endif
        {
            for (auto const& i : m_main) {
                if (i.second.second)
                    batch.Put(ldb::Slice((char const*)i.first.data(), i.first.size), ldb::Slice(i.second.first.data(), i.second.first.size()));
                //				cnote << i.first << "#" << m_main[i.first].second;
            }
            for (auto const& i : m_aux)
                if (i.second.second) {
                    bytes b = i.first.asBytes();
                    b.push_back(255); // for aux
                    batch.Put(bytesConstRef(&b), bytesConstRef(&i.second.first));
                }
        }

        for (unsigned i = 0; i < 10; ++i) {
            ldb::Status o = m_db->Write(m_writeOptions, &batch);
            if (o.ok())
                break;
            if (i == 9) {
                cwarn << "Fail writing to state database. Bombing out.";
                exit(-1);
            }
            cwarn << "Error writing to state database: " << o.ToString();
            WriteBatchNoter n;
            batch.Iterate(&n);
            cwarn << "Sleeping for" << (i + 1) << "seconds, then retrying.";
            std::this_thread::sleep_for(std::chrono::seconds(i + 1));
        }
#if DEV_GUARDED_DB
        DEV_WRITE_GUARDED(x_this)
#endif
        {
            m_aux.clear();
            m_main.clear();
        }
    }
}

bytes OverlayDB::lookupAux(h256 const& _h) const
{
    bytes ret = MemoryDB::lookupAux(_h);
    if (!ret.empty() || !m_db)
        return ret;
    std::string v;
    
    bytes b = _h.asBytes();
    b.push_back(255); // for aux
    m_db->Get(m_readOptions, bytesConstRef(&b), &v);
    if (v.empty())
            cwarn << "Aux not found: " << _h;
    return asBytes(v);
}

void OverlayDB::rollback()
{
#if DEV_GUARDED_DB
    WriteGuard l(x_this);
#endif
    m_main.clear();
}

std::string OverlayDB::lookup(h256 const& _h) const
{
    std::string ret = MemoryDB::lookup(_h);
    if (ret.empty() && m_db)
        m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
    return ret;
}

bool OverlayDB::exists(h256 const& _h) const
{
    if (MemoryDB::exists(_h))
        return true;
    std::string ret;
    if (m_db)
        m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
    return !ret.empty();
}

void OverlayDB::kill(h256 const& _h)
{
#if ETH_PARANOIA || 1
    if (!MemoryDB::kill(_h)) {
        std::string ret;
        if (m_db)
            m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
        // No point node ref decreasing for EmptyTrie since we never bother incrementing it in the first place for
        // empty storage tries.
        if (ret.empty() && _h != EmptyTrie)
            cnote << "Decreasing DB node ref count below zero with no DB node. Probably have a corrupt Trie." << _h;

        // TODO: for 1.1: ref-counted triedb.
    }
#else
    MemoryDB::kill(_h);
#endif
}

bool OverlayDB::deepkill(h256 const& _h)
{
    // kill in memoryDB
    kill(_h);

    //kill in overlayDB
    ldb::Status s = m_db->Delete(m_writeOptions, ldb::Slice((char const*)_h.data(), 32));
    if (s.ok())
        return true;
    else
        return false;
}


// ====== TrieCommon  =======

std::string hexPrefixEncode(bytes const& _hexVector, bool _leaf, int _begin, int _end)
{
    unsigned begin = _begin;
    unsigned end = _end < 0 ? _hexVector.size() + 1 + _end : _end;
    bool odd = ((end - begin) % 2) != 0;

    std::string ret(1, ((_leaf ? 2 : 0) | (odd ? 1 : 0)) * 16);
    if (odd) {
        ret[0] |= _hexVector[begin];
        ++begin;
    }
    for (unsigned i = begin; i < end; i += 2)
        ret += _hexVector[i] * 16 + _hexVector[i + 1];
    return ret;
}

std::string hexPrefixEncode(bytesConstRef _data, bool _leaf, int _beginNibble, int _endNibble, unsigned _offset)
{
    unsigned begin = _beginNibble + _offset;
    unsigned end = (_endNibble < 0 ? ((int)(_data.size() * 2 - _offset) + 1) + _endNibble : _endNibble) + _offset;
    bool odd = (end - begin) & 1;

    std::string ret(1, ((_leaf ? 2 : 0) | (odd ? 1 : 0)) * 16);
    ret.reserve((end - begin) / 2 + 1);

    unsigned d = odd ? 1 : 2;
    for (auto i = begin; i < end; ++i, ++d) {
        byte n = nibble(_data, i);
        if (d & 1)           // odd
            ret.back() |= n; // or the nibble onto the back
        else
            ret.push_back(n << 4); // push the nibble on to the back << 4
    }
    return ret;
}

std::string hexPrefixEncode(bytesConstRef _d1, unsigned _o1, bytesConstRef _d2, unsigned _o2, bool _leaf)
{
    unsigned begin1 = _o1;
    unsigned end1 = _d1.size() * 2;
    unsigned begin2 = _o2;
    unsigned end2 = _d2.size() * 2;

    bool odd = (end1 - begin1 + end2 - begin2) & 1;

    std::string ret(1, ((_leaf ? 2 : 0) | (odd ? 1 : 0)) * 16);
    ret.reserve((end1 - begin1 + end2 - begin2) / 2 + 1);

    unsigned d = odd ? 1 : 2;
    for (auto i = begin1; i < end1; ++i, ++d) {
        byte n = nibble(_d1, i);
        if (d & 1)           // odd
            ret.back() |= n; // or the nibble onto the back
        else
            ret.push_back(n << 4); // push the nibble on to the back << 4
    }
    for (auto i = begin2; i < end2; ++i, ++d) {
        byte n = nibble(_d2, i);
        if (d & 1)           // odd
            ret.back() |= n; // or the nibble onto the back
        else
            ret.push_back(n << 4); // push the nibble on to the back << 4
    }
    return ret;
}

byte uniqueInUse(RLP const& _orig, byte except)
{
    byte used = 255;
    for (unsigned i = 0; i < 17; ++i)
        if (i != except && !_orig[i].isEmpty()) {
            if (used == 255)
                used = (byte)i;
            else
                return 255;
        }
    return used;
}


// ====== TrieHash  =======


/*/
#define APPEND_CHILD appendData
/*/
#define APPEND_CHILD appendRaw
/**/

#define ENABLE_DEBUG_PRINT 0

#if ENABLE_DEBUG_PRINT
bool g_hashDebug = false;
#endif

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp);

void hash256rlp(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
#if ENABLE_DEBUG_PRINT
    static std::string s_indent;
    if (_preLen)
        s_indent += "  ";
#endif

    if (_begin == _end)
        _rlp << ""; // NULL
    else if (std::next(_begin) == _end) {
        // only one left - terminate with the pair.
        _rlp.appendList(2) << hexPrefixEncode(_begin->first, true, _preLen) << _begin->second;
#if ENABLE_DEBUG_PRINT
        if (g_hashDebug)
            std::cerr << s_indent << toHex(bytesConstRef(_begin->first.data() + _preLen, _begin->first.size() - _preLen), 1) << ": " << _begin->second << " = " << sha3(_rlp.out()) << std::endl;
#endif
    } else {
        // find the number of common prefix nibbles shared
        // i.e. the minimum number of nibbles shared at the beginning between the first hex string and each successive.
        unsigned sharedPre = (unsigned)-1;
        unsigned c = 0;
        for (auto i = std::next(_begin); i != _end && sharedPre; ++i, ++c) {
            unsigned x = std::min(sharedPre, std::min((unsigned)_begin->first.size(), (unsigned)i->first.size()));
            unsigned shared = _preLen;
            for (; shared < x && _begin->first[shared] == i->first[shared]; ++shared) {
            }
            sharedPre = std::min(shared, sharedPre);
        }
        if (sharedPre > _preLen) {
        // if they all have the same next nibble, we also want a pair.
#if ENABLE_DEBUG_PRINT
            if (g_hashDebug)
                std::cerr << s_indent << toHex(bytesConstRef(_begin->first.data() + _preLen, sharedPre), 1) << ": " << std::endl;
#endif
            _rlp.appendList(2) << hexPrefixEncode(_begin->first, false, _preLen, (int)sharedPre);
            hash256aux(_s, _begin, _end, (unsigned)sharedPre, _rlp);
#if ENABLE_DEBUG_PRINT
            if (g_hashDebug)
                std::cerr << s_indent << "= " << hex << sha3(_rlp.out()) << dec << std::endl;
#endif
        } else {
            // otherwise enumerate all 16+1 entries.
            _rlp.appendList(17);
            auto b = _begin;
            if (_preLen == b->first.size()) {
#if ENABLE_DEBUG_PRINT
                if (g_hashDebug)
                    std::cerr << s_indent << "@: " << b->second << std::endl;
#endif
                ++b;
            }
            for (auto i = 0; i < 16; ++i) {
                auto n = b;
                for (; n != _end && n->first[_preLen] == i; ++n) {
                }
                if (b == n)
                    _rlp << "";
                else {
#if ENABLE_DEBUG_PRINT
                    if (g_hashDebug)
                        std::cerr << s_indent << std::hex << i << ": " << std::dec << std::endl;
#endif
                    hash256aux(_s, b, n, _preLen + 1, _rlp);
                }
                b = n;
            }
            if (_preLen == _begin->first.size())
                _rlp << _begin->second;
            else
                _rlp << "";

#if ENABLE_DEBUG_PRINT
            if (g_hashDebug)
                std::cerr << s_indent << "= " << hex << sha3(_rlp.out()) << dec << std::endl;
#endif
        }
    }
#if ENABLE_DEBUG_PRINT
    if (_preLen)
        s_indent.resize(s_indent.size() - 2);
#endif
}

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
    RLPStream rlp;
    hash256rlp(_s, _begin, _end, _preLen, rlp);
    if (rlp.out().size() < 32) {
    // RECURSIVE RLP
#if ENABLE_DEBUG_PRINT
        cerr << "[INLINE: " << dec << rlp.out().size() << " < 32]" << endl;
#endif
        _rlp.APPEND_CHILD(rlp.out());
    } else {
#if ENABLE_DEBUG_PRINT
        cerr << "[HASH: " << dec << rlp.out().size() << " >= 32]" << endl;
#endif
        _rlp << sha3(rlp.out());
    }
}

bytes asNibbles(bytesConstRef const& _s)
{
    std::vector<uint8_t> ret;
    ret.reserve(_s.size() * 2);
    for (auto i : _s) {
        ret.push_back(i / 16);
        ret.push_back(i % 16);
    }
    return ret;
}

bytes rlp256(BytesMap const& _s)
{
    // build patricia tree.
    if (_s.empty())
        return rlp("");
    HexMap hexMap;
    for (auto i = _s.rbegin(); i != _s.rend(); ++i)
        hexMap[asNibbles(bytesConstRef(&i->first))] = i->second;
    RLPStream s;
    hash256rlp(hexMap, hexMap.cbegin(), hexMap.cend(), 0, s);
    return s.out();
}

h256 hash256(BytesMap const& _s)
{
    return sha3(rlp256(_s));
}

h256 orderedTrieRoot(std::vector<bytes> const& _data)
{
    BytesMap m;
    unsigned j = 0;
    for (auto i : _data)
        m[rlp(j++)] = i;
    return hash256(m);
}

h256 orderedTrieRoot(std::vector<bytesConstRef> const& _data)
{
    BytesMap m;
    unsigned j = 0;
    for (auto i : _data)
        m[rlp(j++)] = i.toBytes();
    return hash256(m);
}


// ====== TrieDB  =======
h256 const c_shaNull = sha3(rlp(""));
const char* TrieDBChannel::name() { return "-T-"; }

// ====== Defaults  =======
Defaults* Defaults::s_this = nullptr;

Defaults::Defaults()
{
    m_dbPath = getDataDir();
}

} // namespace sc
