#ifndef BITCOINJS_SERVER_INCLUDE_COINSDB_H_
#define BITCOINJS_SERVER_INCLUDE_COINSDB_H_

#include <v8.h>
#include <node.h>

#include <sparsehash/sparse_hash_map>

#include "util/inttypes.h"
#include "util/serialize.h"
#include "util/slice.h"
#include "util/hash.h"
#include "util/stream.h"
#include "util/varint.h"

using namespace v8;
using namespace node;
using google::sparse_hash_map;

namespace bitcoinjs {

class TxOut : public Serializable<TxOut>
{
 public:
  int64_t value;
  std::string script;

  TxOut()
  {
    SetNull();
  }

  TxOut(const TxOut &o)
    : value(o.value), script(o.script)
  {
  }

  void SetNull()
  {
    value = -1;
    script = "";
  }

  bool IsNull() const
  {
    return (this == NULL || value == -1);
  }

  friend bool operator==(const TxOut& a, const TxOut& b)
  {
    return (a.value  == b.value &&
            a.script == b.script);
  }

  friend bool operator!=(const TxOut& a, const TxOut& b)
  {
    return !(a == b);
  }

  void Serialize(std::ostream &os) const
  {
    os << IntTool::SerializeLE(value) << VarInt(script.size()) << script;
  }

  void Unserialize(Parser &p)
  {
    value = p.Uint64();
    VarInt scriptLen = p.Parse<VarInt>();
    script = p.Str(scriptLen);
  }
};

class Coins : public Serializable<Coins>
{
public:
  Coins() : outs(0) {}
  Coins(const Coins &c) : outs(c.outs) {}

  void Resize(size_t count)
  {
    outs.resize(count);
  }

  void Set(size_t pos, TxOut o)
  {
    if (pos >= outs.size()) {
      Resize(pos + 1);
    }
    outs[pos] = o;
  }

  void Cleanup()
  {
    while (outs.size() > 0 && outs.back().IsNull()) {
      outs.pop_back();
    }
  }

  TxOut Get(size_t pos)
  {
    TxOut out;
    if (pos <= outs.size() && !outs[pos].IsNull()) {
      out = outs[pos];
    }
    return out;
  }

  bool Spend(size_t pos)
  {
    if (pos > outs.size() || outs[pos].IsNull())
      return false;

    outs[pos].SetNull();
    Cleanup();

    return true;
  }

  bool IsAvailable(size_t pos) const
  {
    return (pos < outs.size()) && !outs[pos].IsNull();
  }

  bool IsEmpty() const
  {
    for (std::vector<TxOut>::const_iterator it = outs.begin();
         it != outs.end(); ++it) {
      if (!it->IsNull()) {
        return false;
      }
    }
    return true;
  }

  size_t Length() const
  {
    return outs.size();
  }

  void Serialize(std::ostream &os) const
  {
    os << VarInt(outs.size());
    for (std::vector<TxOut>::const_iterator it = outs.begin();
         it != outs.end(); ++it) {
      os << *it;
    }
  }

  void Unserialize(Parser &p)
  {
    VarInt size;
    p >> size;
    Resize(size);

    for (uint32_t i = 0; i < size; i++) {
      TxOut o;
      p >> o;
      Set(i, o);
    }
  }

private:
  std::vector<TxOut> outs;
};

class CoinsDB : public ObjectWrap
{
 public:
  CoinsDB();
  CoinsDB(uint32_t seed);
  static Persistent<FunctionTemplate> constructor;
  static void Init(Handle<Object> target);

  void Save(const uint256_t &hash, const Coins &c);
  Coins Get(const uint256_t &hash);

 private:
  static Handle<Value> New(const Arguments& args);

  typedef sparse_hash_map<uint256_t, std::vector<char>, Hash> map_type;
  typedef typename map_type::iterator iterator_type;
  typedef typename map_type::const_iterator const_iterator_type;

  map_type db;
};

} // bitcoinjs

#endif
