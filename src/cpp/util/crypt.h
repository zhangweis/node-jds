#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_CRYPT_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_CRYPT_H_

#include <cstring>
#include "inttypes.h"

namespace bitcoinjs {

class Crypt {
 public:
  static void HashDoubleSha256(const char *data, size_t len, uint256_t *hash);
};

} // bitcoinjs

#endif
