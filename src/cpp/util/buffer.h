#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_BUFFER_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_BUFFER_H_

#include <node.h>

namespace bitcoinjs {

class BufferTool
{
 public:
  static Handle<Value> FromCharArray(char *data, size_t len)
  {
    HandleScope scope;
    Buffer *slowBuffer = Buffer::New(data, len, &DeleteCharArray, NULL);

    Local<Object> global = Context::GetCurrent()->Global();
    Local<Value> bv = global->Get(String::NewSymbol("Buffer"));
    Local<Function> b = Local<Function>::Cast(bv);

    Handle<Value> argv[3] = { slowBuffer->handle_, Integer::New(len), Integer::New(0) };
    Local<Object> fastBuffer = b->NewInstance(3, argv);

    return scope.Close(fastBuffer);
  }

 private:
  static void DeleteCharArray(char *data, void *hint)
  {
    delete[] data;
  }
};

} // bitcoinjs

#endif
