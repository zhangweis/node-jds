#ifndef BITCOINJS_SERVER_INCLUDE_COMMON_H_
#define BITCOINJS_SERVER_INCLUDE_COMMON_H_

#include <v8.h>

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                             \
    return ThrowException(Exception::TypeError(                                   \
                            String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

static v8::Handle<v8::Value> VException(const char *msg) {
    v8::HandleScope scope;
    return v8::ThrowException(v8::Exception::Error(v8::String::New(msg)));
}

#endif
