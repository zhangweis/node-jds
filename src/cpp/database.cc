
#include <stdint.h>
#include <cstdlib>
#include <exception>

// node_leveldb
#include "helpers.h"
#include "handle.h"
#include "batch.h"

#include "util/crypt.h"
#include "util/hex.h"
#include "util/inttypes.h"
#include "util/parser.h"
#include "util/varstr.h"
#include "util/buffer.h"

#include "database.h"

using namespace node_leveldb;

namespace bitcoinjs {

Persistent<FunctionTemplate> Database::constructor;

Database::Database(leveldb::DB* db)
  : ObjectWrap()
  , db_(db)
{
  // TODO: Keep handle to node_leveldb object.
  coinsdb_ = new CoinsDB;
}

Database::~Database()
{
  // The leveldb::DB object will be freed by the node_leveldb module
  delete coinsdb_;
}

class Database::OpAsync {
 public:
  OpAsync(const Handle<Value>& callback)
    : err_(), status_(leveldb::Status())
  {
    assert(callback->IsFunction());
    Handle<Function> cb = Handle<Function>::Cast(callback);
    callback_ = Persistent<Function>::New(cb);
  }

  virtual ~OpAsync()
  {
    callback_.Dispose();
  }

  template <class T> static Handle<Value> AsyncEnqueue(T* op)
  {
    return AsyncQueue(op, AsyncWorker<T>, AsyncCallback<T>);
  }

  template <class T> static void AsyncWorker(uv_work_t* req)
  {
    T* op = static_cast<T*>(req->data);
    try {
      op->Run();
    } catch (std::exception &e) {
      op->err_ = e.what();
    }
  }

  template <class T> static void AsyncCallback(uv_work_t* req)
  {
    HandleScope scope;
    T* op = static_cast<T*>(req->data);
    assert(!op->callback_.IsEmpty());

    Handle<Value> error = Null();
    Handle<Value> result = Null();

    op->Result(error, result);

    if ((error.IsEmpty() || error->IsNull()) && (result.IsEmpty() || result->IsNull()) &&
        !op->err_.empty()) {
      std::string cppError = "Native: " + op->err_;
      error = Exception::Error(String::New(cppError.c_str(), cppError.length()));
    }

    if (error.IsEmpty() && result.IsEmpty() && !op->status_.ok())
      error = Exception::Error(String::New(op->status_.ToString().c_str()));

    Handle<Value> args[] = { error, result };

    TryCatch tryCatch;
    op->callback_->Call(Context::GetCurrent()->Global(), 2, args);
    if (tryCatch.HasCaught()) FatalException(tryCatch);

    delete op;
    delete req;
  }
  std::string err_;
  leveldb::Status status_;
  Persistent<Function> callback_;
};


class Database::AddBlockAsync : public OpAsync {
 public:
  AddBlockAsync(const Handle<Value>& callback) : OpAsync(callback) {}
  virtual ~AddBlockAsync() { batchHandle_.Dispose(); }

  static Handle<Value> Hook(const Arguments& args)
  {
    HandleScope scope;

    if (args.Length() != 3) {
      return ThrowTypeError("Database::addBlock expects 3 arguments");
    }
    if (!Buffer::HasInstance(args[0])) {
      return ThrowTypeError("Database::addBlock(): First argument must be a Buffer");
    }
    if (!JBatch::HasInstance(args[1])) {
      return ThrowTypeError("Database::addBlock(): Second argument must be a WriteBatch");
    }
    if (!args[2]->IsFunction()) {
      return ThrowTypeError("Database::addBlock(): Third argument must be a callback function");
    }

    AddBlockAsync* op = new AddBlockAsync(args[2]);

    // Required self
    op->self_ = Unwrap<Database>(args.This());

    // Required buffer
    op->buf_ = Buffer::Data(args[0]->ToObject());
    op->bufLen_ = Buffer::Length(args[0]->ToObject());
    op->bufHandle_ = Persistent<Value>::New(args[0]);

    // Required batch
    op->batch_ = Unwrap<JBatch>(args[1]->ToObject())->GetBatch();
    op->batchHandle_ = Persistent<Value>::New(args[1]);

    return AsyncEnqueue<AddBlockAsync>(op);
  }

  void Run()
  {
      Parser p((unsigned char *) buf_, bufLen_);
      uint32_t i, j, tx_start, tx_len, outPos;
      uint64_t n_tx, n_txin, n_txout, script_len;
      uint256_t *block_hash, prev_hash;
      leveldb::Slice key, value;
      Coins coinsOut, coinsIn;

      // Skip block header
      p.Seek(80);

      // Get tx count so we can reserve memory for the hashes
      n_tx = p.Parse<VarInt>();

      uint256_t hashes[n_tx];

      // We've allocated space for one extra hash, use that for the block hash
      block_hash = &hashes[n_tx];

      // Calculate block hash
      Crypt::HashDoubleSha256(buf_, 80, block_hash);

      // Save block header
      key = leveldb::Slice((char*)block_hash, 32);
      value = leveldb::Slice(buf_, 80);
      batch_->Put(key, value);

      for (i = 0; i < n_tx; i++) {
        tx_start = p.Tell();

        // Skip tx version
        p.Seek(4);

        n_txin = p.Parse<VarInt>();
        while (n_txin > 0) {
          // Mark outpoint spent by this transaction
          if (i != 0) {
            // LevelDB won't try to read the transaction hash until the WriteBatch
            // is finalized, so it'll be set in time.
            key = leveldb::Slice(buf_ + p.Tell(), 36);
            value = leveldb::Slice((char*)&hashes[i], 32);
            batch_->Put(key, value);

            prev_hash = p.Uint256();
            coinsIn = self_->coinsdb_->Get(prev_hash);
            outPos = p.Uint32();
            coinsIn.Spend(outPos);
            self_->coinsdb_->Save(prev_hash, coinsIn);
          } else {
            p.Seek(36);
          }

          // Skip script
          script_len = p.Parse<VarInt>();
          p.Seek(script_len + 4);
          n_txin--;
        }

        // Skip txouts
        n_txout = p.Parse<VarInt>();
        coinsOut.Resize(n_txout);
        for (j = 0; j < n_txout; j++) {
          TxOut txo = p.Parse<TxOut>();
          coinsOut.Set(j, txo);
        }

        // Skip lock_time
        p.Seek(4);

        tx_len = p.Tell() - tx_start;

        // Calculate tx hash
        Crypt::HashDoubleSha256(buf_ + tx_start, tx_len, &hashes[i]);

        // Store tx
        key = leveldb::Slice((char*)&hashes[i], 32);
        value = leveldb::Slice(buf_ + tx_start, tx_len);
        batch_->Put(key, value);

        // Save outputs
        self_->coinsdb_->Save(hashes[i], coinsOut);
      }

      // Save to database
      status_ = self_->db_->Write(leveldb::WriteOptions(), batch_);
  }

  void Result(Handle<Value>& error, Handle<Value>& result)
  {
  }

  Database* self_;

  const char* buf_;
  size_t bufLen_;
  Persistent<Value> bufHandle_;

  leveldb::WriteBatch* batch_;
  Persistent<Value> batchHandle_;
};

Handle<Value> Database::GetOutputsByInputs(const Arguments& args)
{
  HandleScope scope;

  try {

    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    if (args.Length() != 1) {
      return VException("One argument expected: inputs");
    }
    if (!args[0]->IsArray()) {
      return VException("Argument 'inputs' must be of type Array");
    }
    Handle<Array> inputs = Handle<Array>::Cast(args[0]);
    Local<Array> outputs = Array::New();

    for (uint32_t i = 0; i < inputs->Length(); i++) {
      Handle<Value> input = inputs->Get(i);
      if (!Buffer::HasInstance(input)) {
        return VException("Inputs must be given as buffers");
      }
      Handle<Object> inputBuffer = input->ToObject();

      Parser p((uint8_t*)Buffer::Data(inputBuffer), Buffer::Length(inputBuffer));

      uint256_t hash = p.Uint256();
      uint32_t pos = p.Uint32();

      Coins coins = db->coinsdb_->Get(hash);
      TxOut out = coins.Get(pos);

      if (!out.IsNull()) {
        size_t len = out.GetSerializeSize();
        char *data = new char[len];

        // Serialize TxOut
        MemStream ms(data, len);
        ms << out;

        // BufferTool creates a Buffer object that will automatically
        // delete the char[] when it's no longer needed.
        Local<Value> buffer =
          Local<Value>::New(BufferTool::FromCharArray(data, len));

        outputs->Set(Number::New(i), buffer);
      } else {
        outputs->Set(Number::New(i), Null());
      }
    }

    return scope.Close(outputs);
  } catch (std::exception &e) {
    std::ostringstream s;
    s << "Native: " << e.what();
    return ThrowException(Exception::Error(String::New(s.str().c_str(), s.str().size())));
  }
};

void Database::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor = Persistent<FunctionTemplate>::New(t);
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("Database"));

  // Instance methods
  NODE_SET_PROTOTYPE_METHOD(constructor, "addBlock", AddBlockAsync::Hook);
  NODE_SET_PROTOTYPE_METHOD(constructor, "getOutputsByInputs", GetOutputsByInputs);

  // Static methods
  // <none>

  target->Set(String::NewSymbol("Database"),
              constructor->GetFunction());
}


Handle<Value> Database::New(const Arguments& args)
{
  HandleScope scope;

  assert(args.Length() == 1);
  assert(args[0]->IsObject());

  if (!JHandle::HasInstance(args[0])) {
    return ThrowTypeError("Database(): Argument must be a leveldb Handle");
  }

  JHandle* hnd = Unwrap<JHandle>(args[0]->ToObject());
  leveldb::DB* ldb = hnd->GetDB();
  Database* self = new Database(ldb);

  self->Wrap(args.This());

  return args.This();
}

} // bitcoinjs
