#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_HEX_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_HEX_H_

#include <string>
#include <vector>
#include "inttypes.h"

namespace bitcoinjs {

class Hex {
 public:
  static std::string ToHex(const char *data, size_t size);
  static std::string ToHex(std::string s);
  static std::string ToHex(std::vector<char> v);
  static std::string ToHex(uint256_t hash);
};

} // bitcoinjs

#endif
