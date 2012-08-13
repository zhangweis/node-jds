#include <sstream>

#include "coinsdb.h"

#include "util/hex.h"

namespace bitcoinjs {

Persistent<FunctionTemplate> CoinsDB::constructor;

CoinsDB::CoinsDB() : db()
{
  uint256_t nullkey;
  db.set_deleted_key(nullkey);
}

CoinsDB::CoinsDB(uint32_t seed) : db(0, seed)
{
}

void CoinsDB::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor = Persistent<FunctionTemplate>::New(t);
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("CoinsDB"));

  // Instance methods
  // <none>

  // Static methods
  // <none>

  target->Set(String::NewSymbol("CoinsDB"),
              constructor->GetFunction());
}

Handle<Value> CoinsDB::New(const Arguments& args)
{
  HandleScope scope;

  CoinsDB* self = new CoinsDB;

  self->Wrap(args.This());

  return args.This();
}

void CoinsDB::Save(const uint256_t &hash, const Coins &c)
{
  if (c.IsEmpty()) {
    db.erase(hash);
  } else {
    VectorStream v;
    v << c;
    db[hash].swap(v.VecRef());
  }
}

Coins CoinsDB::Get(const uint256_t &hash)
{
  Coins c;
  const_iterator_type it;

  it = db.find(hash);

  // If the entry is not found or empty, we return a null Coins
  if (it == db.end() || !it->second.size()) return c;

  Parser p(it->second);
  c.Unserialize(p);

  return c;
}


} // bitcoinjs
