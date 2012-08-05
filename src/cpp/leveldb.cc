
#include <stdint.h>
#include <cstdlib>

#include "../../node_modules/leveldb/src/cpp/helpers.h"
#include "util/crypt.h"
#include "util/hex.h"
#include "util/inttypes.h"
#include "util/parser.h"

#include "leveldb.h"

using namespace node_leveldb;

Persistent<FunctionTemplate> LevelDB::constructor;

LevelDB::LevelDB(leveldb::DB* db)
  : ObjectWrap()
  , db_(db)
{
}

class LevelDB::OpAsync {
 public:
  OpAsync(const Handle<Value>& callback)
    : status_(leveldb::Status())
  {
    assert(callback->IsFunction());
    Handle<Function> cb = Handle<Function>::Cast(callback);
    callback_ = Persistent<Function>::New(cb);
  }

  virtual ~OpAsync() {
    callback_.Dispose();
  }

  template <class T> static Handle<Value> AsyncEnqueue(T* op) {
    return AsyncQueue(op, AsyncWorker<T>, AsyncCallback<T>);
  }

  template <class T> static void AsyncWorker(uv_work_t* req) {
    T* op = static_cast<T*>(req->data);
    op->Run();
  }

  template <class T> static void AsyncCallback(uv_work_t* req) {
    HandleScope scope;
    T* op = static_cast<T*>(req->data);
    assert(!op->callback_.IsEmpty());

    Handle<Value> error = Null();
    Handle<Value> result = Null();

    op->Result(error, result);

    if (error.IsEmpty() && result.IsEmpty() && !op->status_.ok())
      error = Exception::Error(String::New(op->status_.ToString().c_str()));

    Handle<Value> args[] = { error, result };

    TryCatch tryCatch;
    op->callback_->Call(Context::GetCurrent()->Global(), 2, args);
    if (tryCatch.HasCaught()) FatalException(tryCatch);

    delete op;
    delete req;
  }
  leveldb::Status status_;
  Persistent<Function> callback_;
};


class LevelDB::AddBlockAsync : public OpAsync {
 public:
  AddBlockAsync(const Handle<Value>& callback) : OpAsync(callback) {}
  virtual ~AddBlockAsync() { batchHandle_.Dispose(); }

  static Handle<Value> Hook(const Arguments& args) {
    HandleScope scope;

    if (args.Length() != 3) {
      return ThrowTypeError("BitcoinLevelDB::addBlock expects 3 arguments");
    }
    if (!Buffer::HasInstance(args[0])) {
      return ThrowTypeError("BitcoinLevelDB::addBlock(): First argument must be a Buffer");
    }
    if (!JBatch::HasInstance(args[1])) {
      return ThrowTypeError("BitcoinLevelDB::addBlock(): Second argument must be a WriteBatch");
    }
    if (!args[2]->IsFunction()) {
      return ThrowTypeError("BitcoinLevelDB::addBlock(): Third argument must be a callback function");
    }

    AddBlockAsync* op = new AddBlockAsync(args[2]);

    // Required self
    op->self_ = Unwrap<LevelDB>(args.This());

    // Required buffer
    op->buf_ = Buffer::Data(args[0]->ToObject());
    op->bufLen_ = Buffer::Length(args[0]->ToObject());
    op->bufHandle_ = Persistent<Value>::New(args[0]);

    // Required batch
    op->batch_ = Unwrap<JBatch>(args[1]->ToObject())->GetBatch();
    op->batchHandle_ = Persistent<Value>::New(args[1]);

    return AsyncEnqueue<AddBlockAsync>(op);
  }

  void Run() {
    Parser p((unsigned char *) buf_, bufLen_);
    uint32_t i, tx_start, tx_len;
    uint64_t n_tx, n_txin, n_txout, script_len;
    uint256_t *block_hash, *tx_hashes = NULL;
    leveldb::Slice key, value;

    // Skip block header
    p.Seek(80);

    // Get tx count so we can reserve memory for the hashes
    n_tx = p.VarInt();
    tx_hashes = (uint256_t*)malloc(sizeof(uint256_t) * (n_tx + 1));

    // We've allocated one extra hash, we'll borrow that for the block hash
    block_hash = &tx_hashes[n_tx];

    // Calculate block hash
    Crypt::HashDoubleSha256(buf_, 80, block_hash);
    //block_hash->ReverseBytes();

    // Save block header
    key = leveldb::Slice((char*)block_hash, 32);
    value = leveldb::Slice(buf_, 80);
    batch_->Put(key, value);

    for (i = 0; i < n_tx; i++) {
      tx_start = p.Tell();

      // Skip tx version
      p.Seek(4);

      n_txin = p.VarInt();
      while (n_txin > 0) {
        // Mark outpoint spent by this transaction
        if (i != 0) {
          // LevelDB won't try to read the transaction hash until the WriteBatch
          // is finalized, so it'll be set in time.
          key = leveldb::Slice(buf_ + p.Tell(), 36);
          value = leveldb::Slice((char*)&tx_hashes[i], 32);
          batch_->Put(key, value);
        }
        p.Seek(36);

        // Skip script
        script_len = p.VarInt();
        p.Seek(script_len + 4);
        n_txin--;
      }

      // Skip txouts
      n_txout = p.VarInt();
      while (n_txout > 0) {
        p.Seek(8);
        script_len = p.VarInt();
        p.Seek(script_len);
        n_txout--;
      }

      // Skip lock_time
      p.Seek(4);

      tx_len = p.Tell() - tx_start;

      // Calculate tx hash
      Crypt::HashDoubleSha256(buf_ + tx_start, tx_len, &tx_hashes[i]);

      // Store tx
      key = leveldb::Slice((char*)&tx_hashes[i], 32);
      value = leveldb::Slice(buf_ + tx_start, tx_len);
      batch_->Put(key, value);
    }

    // Save to database
    status_ = self_->db_->Write(leveldb::WriteOptions(), batch_);

    // The WriteBatch has completed, it's safe to deallocate the hashes
    free(tx_hashes);
  }

  void Result(Handle<Value>& error, Handle<Value>& result) {
  }

  LevelDB* self_;

  const char* buf_;
  size_t bufLen_;
  Persistent<Value> bufHandle_;

  leveldb::WriteBatch* batch_;
  Persistent<Value> batchHandle_;
};

void LevelDB::Init(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor = Persistent<FunctionTemplate>::New(t);
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("LevelDB"));

  // Instance methods
  NODE_SET_PROTOTYPE_METHOD(constructor, "addBlock", AddBlockAsync::Hook);

  // Static methods
  // <none>

  target->Set(String::NewSymbol("LevelDB"),
              constructor->GetFunction());
}


Handle<Value> LevelDB::New(const Arguments& args) {
  HandleScope scope;

  assert(args.Length() == 1);
  assert(args[0]->IsObject());

  if (!JHandle::HasInstance(args[0])) {
    return ThrowTypeError("BitcoinLevelDB(): Argument must be a leveldb Handle");
  }

  JHandle* hnd = Unwrap<JHandle>(args[0]->ToObject());
  leveldb::DB* ldb = hnd->GetDB();
  LevelDB* self = new LevelDB(ldb);

  self->Wrap(args.This());

  return args.This();
}
