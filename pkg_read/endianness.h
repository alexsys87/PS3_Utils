#pragma once

#include <stdint.h>
#include <stdlib.h>

//inline uint16_t be16(uint16_t n) { return(_byteswap_ushort(n)); }

//inline uint32_t be32(uint32_t n) { return(_byteswap_ulong(n)); }

//inline uint64_t be64(uint64_t n) { return(_byteswap_uint64(n)); }

inline uint16_t be16(uint16_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(x);
#endif
#ifdef _MSC_VER
    return _byteswap_ushort(x);
#endif
    return (((x) >> 8) & 0xff) | (((x) & 0xff) << 8);
}

inline uint32_t be32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(x);
#endif
#ifdef _MSC_VER
    return _byteswap_ulong(x);
#endif
    return  (((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24);
}

inline uint64_t be64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(x);
#endif
#ifdef _MSC_VER
    return _byteswap_uint64(x);
#endif
    return  (((x) & 0xff00000000000000ull) >> 56) | (((x) & 0x00ff000000000000ull) >> 40) | (((x) & 0x0000ff0000000000ull) >> 24) | (((x) & 0x000000ff00000000ull) >>  8)|
            (((x) & 0x00000000ff000000ull) <<  8) | (((x) & 0x0000000000ff0000ull) << 24) | (((x) & 0x000000000000ff00ull) << 40) | (((x) & 0x00000000000000ffull) << 56);
}