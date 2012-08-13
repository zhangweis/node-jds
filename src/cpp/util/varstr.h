#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_VARSTR_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_VARSTR_H_

#include "serialize.h"
#include "varint.h"

namespace bitcoinjs {

class VarStr : Serializable<VarStr>
{
 public:
  static const unsigned int VARINT_MAX = 0x02000000;

  std::string value;

  VarStr() : value("") {}
  VarStr(const std::string &v) : value(v) {}

  operator std::string() const
  {
    return value;
  }

  void Serialize(std::ostream &os) const
  {
    os << VarInt(value.size());
    os.write(value.c_str(), value.size());
  }

  void Unserialize(Parser &p)
  {
    uint64_t size = p.Parse<VarInt>();
    value = std::string(size, '\0');
    
  }
};

} // bitcoinjs

#endif
