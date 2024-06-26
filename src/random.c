#include "c_types.h"
#include "random.h"

static uint64  rand(uint64 seed, uint64 m);
static uint64  ror(uint64 value, uint8 bits);
static uintptr getStackAddr();

#pragma optimize("t", on)

void RandBuf(byte* buf, int64 size)
{
    uint64 seed = RandUint64((uint64)(buf));
    for (int64 i = 0; i < size; i++)
    {
        byte b = *(buf + i);
        if (b == 0)
        {
            b = 170;
        }
        seed += ror(seed, b%4);
        seed += b;
    }
    for (int64 i = 0; i < size; i++)
    {
        seed = RandUint64(seed);
        *(buf + i) = (byte)seed;
        seed += seed % 256;
    }
}

byte RandByte(uint64 seed)
{
    return (byte)rand(seed, 256);
}

bool RandBool(uint64 seed)
{
    return (bool)rand(seed, 2);
}

int RandInt(uint64 seed)
{
    return (int)rand(seed, UINT32_MAX);
}

uint RandUint(uint64 seed)
{
    return (uint)rand(seed, UINT32_MAX);
}

int64 RandInt64(uint64 seed)
{
    return (int64)rand(seed, UINT64_MAX);
}

uint64 RandUint64(uint64 seed)
{
    return rand(seed, UINT64_MAX);
}

static uint64 rand(uint64 seed, uint64 m)
{
    if (seed < 4096)
    {
        seed += 4096;
    }
    uint64 a = (uint64)(&ror);
    uint64 c = (uint64)(&getStackAddr);
    for (int i = 0; i < 32; i++)
    {
        a += ror(a, 3);
        c += ror(c, 32);
        a += getStackAddr();
        c += getStackAddr();
        seed += ror(seed + a, 3);
        seed += ror(seed + c, 6);
        seed += ror(seed + m, 9);
        seed += ror(seed, 1);        
        seed += ror(seed, 17);
    }
    if (seed % 2 == 0)
    {
        c++;
    }
    return (a * seed + c) % m;
}

static uint64 ror(uint64 value, uint8 bits)
{
    return value >> bits | value << (64 - bits);
}

#pragma warning(push)
#pragma warning(disable: 4172)
static uintptr getStackAddr()
{
    uint stack = 0;
    return (uintptr)(&stack);
}
#pragma warning(pop)

#pragma optimize("t", off)
