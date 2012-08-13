#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_INTTYPES_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_INTTYPES_H_

#include <algorithm>
#include <cassert>

#if defined(_WIN32) && !defined(__MINGW32__)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;  // NOLINT
typedef unsigned short uint16_t;  // NOLINT
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else

#include <stdint.h>

#endif

namespace bitcoinjs {

typedef __uint128_t uint128_t;

typedef union __bjs_uint160
{
  uint8_t d8[20];
  uint32_t d32[5];

  void ReverseBytes()
  {
    unsigned char *istart = d8, *iend = d8 + 20;
    std::reverse(istart, iend);
  }
} uint160_t;

class uint256_t
{
 public:
  union {
    uint8_t d8[32];
    uint32_t d32[8];
    uint64_t d64[4];
    struct {
      uint64_t lo;
      uint64_t mid;
      uint128_t hi;
    };
  };

  uint256_t() : d64()
  {
  }

  uint256_t(int x) : d64()
  {
    d64[3] = x;
  }
  uint256_t(uint64_t lo, uint64_t mid, uint64_t hi)
    : lo(lo), mid(mid), hi(hi)
  {
  }

  void ReverseBytes()
  {
    unsigned char *istart = d8, *iend = d8 + 32;
    std::reverse(istart, iend);
  }

  bool operator==(uint256_t rhs) const
  {
    return d64[0] == rhs.d64[0] && d64[1] == rhs.d64[1] &&
           d64[2] == rhs.d64[2] && d64[3] == rhs.d64[3];
  }

  friend inline uint256_t operator+(const uint256_t &x, const uint256_t &y)
  {
    uint128_t lo = static_cast<uint128_t>(x.lo) + y.lo;
    uint128_t mid = static_cast<uint128_t>(x.mid) + y.mid + (lo >> 64);
    uint256_t result(lo, mid, x.hi + y.hi + (mid >> 64));
    return result;
  }

  friend inline uint256_t operator*(const uint256_t &x, const uint256_t &y)
  {
    uint128_t t1 = static_cast<uint128_t>(x.lo) * y.lo;
    uint128_t t2 = static_cast<uint128_t>(x.lo) * y.mid;
    uint128_t t3 = x.lo * y.hi;
    uint128_t t4 = static_cast<uint128_t>(x.mid) * y.lo;
    uint128_t t5 = static_cast<uint128_t>(x.mid) * y.mid;
    uint64_t t6 = x.mid * y.hi;
    uint128_t t7 = x.hi * y.lo;
    uint64_t t8 = x.hi * y.mid;

    uint64_t lo = t1;
    uint128_t m1 = (t1 >> 64) + static_cast<uint64_t>(t2);
    uint64_t m2 = m1;
    uint128_t mid = static_cast<uint128_t>(m2) + static_cast<uint64_t>(t4);
    uint128_t hi = (t2 >> 64) + t3 + (t4 >> 64) + t5 +
      (static_cast<uint128_t>(t6) << 64) + t7 +
      (static_cast<uint128_t>(t8) << 64) + (m1 >> 64) + (mid >> 64);

    uint256_t result(lo, mid, hi);

    return result;
  }
};

static const union { unsigned char bytes[2]; uint16_t value; } o16_host_order =
  { { 0, 1 } };
static const union { unsigned char bytes[4]; uint32_t value; } o32_host_order =
  { { 0, 1, 2, 3 } };
static const union { unsigned char bytes[8]; uint64_t value; } o64_host_order =
  { { 0, 1, 2, 3, 4, 5, 6, 7 } };

template<typename T>
struct __bjs_binint {
  T inner;
  __bjs_binint(T n) : inner(n)
  {
  }
};

typedef __bjs_binint<uint64_t> bin_uint64_t;

class IntTool
{
 public:
  static inline uint16_t Swap(uint16_t x)
  {
    return (x >> 8) | (x << 8);
  }
  static inline uint32_t Swap(uint32_t x)
  {
    return __builtin_bswap32(x);
  }
  static inline uint64_t Swap(uint64_t x)
  {
    return __builtin_bswap64(x);
  }

  static inline bool IsLittleEndian16()
  {
    if (o16_host_order.value == 0x0100) {
      return 1;
    } else if (o16_host_order.value == 0x0001) {
      return 0;
    } else {
      assert(false);
    }
  }

  static inline bool IsLittleEndian32()
  {
    if (o32_host_order.value == 0x03020100ul) {
      return 1;
    } else if (o32_host_order.value == 0x00010203ul) {
      return 0;
    } else {
      assert(false);
    }
  }

  static inline bool IsLittleEndian64()
  {
    if (o64_host_order.value == 0x0706050403020100ul) {
      return 1;
    } else if (o64_host_order.value == 0x0001020304050607ul) {
      return 0;
    } else {
      abort();
    }
  }

  static inline bin_uint64_t SerializeLE(uint64_t n)
  {
    if (!IsLittleEndian64()) {
      n = Swap(n);
    }

    return static_cast<bin_uint64_t>(n);
  }
};

} // bitcoinjs

#endif
