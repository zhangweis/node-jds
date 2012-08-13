#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_VARINT_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_VARINT_H_

#include <limits>

#include "serialize.h"

namespace bitcoinjs {

class VarInt : Serializable<VarInt>
{
 public:
  static const unsigned int VARINT_MAX = 0x02000000;

  uint64_t value;

  VarInt() : value(0) {}
  VarInt(uint64_t n) : value(n) {}

  operator uint64_t() const
  {
    return value;
  }

  void Serialize(std::ostream &os) const
  {
    if (value < 0xFD) {
      uint8_t val = value;
      WRITEDATA(os, val);
    } else if (value <= std::numeric_limits<uint16_t>::max()) {
      uint8_t size = 0xFD;
      uint16_t val = value;
      WRITEDATA(os, size);
      WRITEDATA(os, val);
    } else if (value <= std::numeric_limits<uint32_t>::max()) {
      uint8_t size = 0xFE;
      uint32_t val = value;
      WRITEDATA(os, size);
      WRITEDATA(os, val);
    } else {
      uint8_t size = 0xFF;
      uint64_t val = value;
      WRITEDATA(os, size);
      WRITEDATA(os, val);
    }
  }

  void Unserialize(Parser &p)
  {
    uint8_t ch = p.Uint8();
    if (ch < 0xFD) {
      value = ch;
    } else if (ch == 0xFD) {
      value = p.Uint16();
    } else if (ch == 0xFE) {
      value = p.Uint32();
    } else { // must be 0xFF
      value = p.Uint64();
    }
    if (value > VARINT_MAX) {
      throw RuntimeError("VarInt::Unserialize(): VARINT_MAX exceeded");
    }
  }
};

} // bitcoinjs

#endif
