#include "parser.h"

#include <cstdio>

namespace bitcoinjs {

template<typename T>
uint64_t Parser<T>::VarInt()
{
  uint64_t result = 0;
  uint8_t ch = Uint8();
  if (ch < 0xFD) {
    result = ch;
  } else if (ch == 0xFD) {
    result = Uint16();
  } else if (ch == 0xFE) {
    result = Uint32();
  } else { // must be 0xFF
    result = Uint64();
  }
  if (result > VARINT_MAX) {
    throw ParserError("Parser::VarInt(): VARINT_MAX exceeded");
  }
  return result;
}

template<typename T>
Slice Parser<T>::VarStrSlice()
{
  uint64_t len = VarInt();
  Slice s(Data(len), len);

  return s;
}

template<typename T>
std::string& Parser<T>::VarStr()
{
  uint64_t len = VarInt();
  return s(Data(len), len);
}

} // bitcoinjs
