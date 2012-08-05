#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_PARSER_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_PARSER_H_

#include "inttypes.h"
#include <cstring>

class Parser {
 public:
  static const unsigned int VARINT_MAX = 0x02000000;

 Parser(const uint8_t *data, size_t length)
   : data(data), pos(0), length(length) {}

  void Seek(int offset);
  inline size_t Tell() { return pos; }

  uint64_t VarInt();

  const uint8_t *data;
  size_t pos;
  size_t length;
};

#endif
