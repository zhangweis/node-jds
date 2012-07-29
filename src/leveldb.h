#ifndef BITCOINJS_SERVER_INCLUDE_LEVELDB_H_
#define BITCOINJS_SERVER_INCLUDE_LEVELDB_H_

#include "../node_modules/leveldb/src/cpp/handle.h"
#include "../node_modules/leveldb/src/cpp/batch.h"

class LevelDB : public ObjectWrap {
 public:
  static Persistent<FunctionTemplate> constructor;
  static void Init(Handle<Object> target);

 private:
  LevelDB(leveldb::DB* db);

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> GetHandleExternal(const Arguments& args);

  class OpAsync;
  class AddBlockAsync;

  leveldb::DB* db_;
};

#endif
