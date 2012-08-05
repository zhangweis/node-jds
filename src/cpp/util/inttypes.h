#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_INTTYPES_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_INTTYPES_H_

#include <stdint.h>
#include <algorithm>

typedef union __bjs_uint256
{
  uint8_t d8[32];
  uint32_t d32[8];
  uint64_t d64[4];

  void ReverseBytes()
  {
    unsigned char *istart = d8, *iend = d8 + 32;
    std::reverse(istart, iend);
  }
} uint256_t;

#endif
