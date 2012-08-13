#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_SERIALIZE_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_SERIALIZE_H_

#include "slice.h"
#include "stream.h"
#include "parser.h"

#define WRITEDATA(s, obj) s.write((char*)&(obj), sizeof(obj))

namespace bitcoinjs {

template<typename T>
class Serializable
{
 public:
  /**
   * Calculate size by simulating serialization.
   *
   * This function calculates the serialize size of the object by
   * simulating the serialization against a counting stream. This
   * means that even classes that do not explicitly implement a
   * GetSerializeSize() method will support it.
   *
   * However, a specialized implementation will likely be faster in
   * most cases since the Serialize() function may do formatting work
   * that is unnecessary for counting.
   */
  size_t GetSerializeSize() const
  {
    CountStream os;
    static_cast<const T*>(this)->Serialize(os);
    return os.size();
  }

  friend std::ostream& operator<<(std::ostream &os, T const &c)
  {
    c.Serialize(os);
    return os;
  }

  friend Parser& operator>>(Parser &p, T &c)
  {
    c.Unserialize(p);
    return p;
  }

  friend std::istream& operator>>(std::istream &is, T &c)
  {
    Parser p(is);
    c.Unserialize(p);
    return is;
  }

  friend VectorStream& operator>>(VectorStream &vs, T &c)
  {
    Parser p(vs.Data(), vs.Size());
    c.Unserialize(p);
    return vs;
  }

  T& operator<<(Parser &p)
  {
    static_cast<T>(this).Unserialize(p);
    return this;
  }
};

inline std::ostream& operator<<(std::ostream &os, const Slice& s)
{
  os.write(s.data(), s.size());
  return os;
}

template<typename T>
inline std::ostream& operator<<(std::ostream &os, const __bjs_binint<T>& n)
  {
  WRITEDATA(os, n);
  return os;
}

template<typename T>
T Parser::Parse() {
  T v;
  v.Unserialize(*this);
  return v;
}

} // bitcoinjs

#endif
