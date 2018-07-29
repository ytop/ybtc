#include <scrlp.h>
#include <scsha3.h>
namespace sc
{
// ============= misc ==============
std::random_device s_fixedHashEngine;


/*** FIPS202 SHAKE VOFs ***/
defshake(128)
defshake(256)

/*** FIPS202 SHA3 FOFs ***/
defsha3(224)
defsha3(256)
defsha3(384)
defsha3(512)


bool sha3(bytesConstRef _input, bytesRef o_output)
{
    // FIXME: What with unaligned memory?
    if (o_output.size() != 32)
        return false;
    sha3_256(o_output.data(), 32, _input.data(), _input.size());
    return true;
}


h256 EmptySHA3 = sha3(bytesConstRef());
const h256 EmptyTrie = sha3(rlp(""));

/// Convert the given value into h160 (160-bit unsigned integer) using the right 20 bytes.

Address right160(h256 const& _t)
{
    Address ret(20);
    memcpy(ret.data(), _t.data() + 12, 20);
    return ret;
}


// Convert from a 256-bit integer stack/memory entry into a 160-bit Address hash.
// Currently we just pull out the right (low-order in BE) 160-bits.
Address asAddress(u256 _item)
{
    return right160(h256(_item));
}

u256 fromAddress(Address _a)
{
    byte t[20];
    byte* p_a = _a.data();
    for (int i = 0; i < 20; i++)
        t[i] = *(p_a + 19 - i);
    return *((u160*)t);
}

} // namespace sc
