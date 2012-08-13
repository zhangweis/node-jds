#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_HASH_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_HASH_H_

#include "murmur/MurmurHash3.h"

namespace bitcoinjs {

class Hash {
 public:
  Hash(void) : seed(0)
  {
  }

  Hash(uint32_t s) : seed(s)
  {
  }

  size_t operator()(const uint256_t &x) const {
    uint32_t hash;
    MurmurHash3_x86_32(x.d8, 32, seed, &hash);
    return hash;
  }
 private:
  uint32_t seed;
};

} // bitcoinjs

#endif
