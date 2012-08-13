#ifndef BITCOINJS_SERVER_INCLUDE_LEVELDB_H_
#define BITCOINJS_SERVER_INCLUDE_LEVELDB_H_

#include "coinsdb.h"

#include "leveldb/db.h"

namespace bitcoinjs {

class Database : public ObjectWrap {
 public:
  static Persistent<FunctionTemplate> constructor;
  static void Init(Handle<Object> target);

 private:
  Database(leveldb::DB* db);
  ~Database();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> GetHandleExternal(const Arguments& args);
  static Handle<Value> GetOutputsByInputs(const Arguments& args);

  class OpAsync;
  class AddBlockAsync;

  leveldb::DB* db_;
  CoinsDB* coinsdb_;
};

} // bitcoinjs

#endif
