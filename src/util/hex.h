#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_HEX_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_HEX_H_

#include <string>
#include <cstring>
#include "inttypes.h"

class Hex {
 public:
  static std::string ToHex(const char *data, size_t size);
  static std::string ToHex(uint256_t hash);
};

#endif
