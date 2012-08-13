#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_EXCEPTION_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_EXCEPTION_H_

#include <stdexcept>
#include <sstream>

#include <v8.h>

using namespace v8;

namespace bitcoinjs {

class RuntimeError : public std::runtime_error
{
 public:
  RuntimeError()
    : std::runtime_error("")
  {
  }

  RuntimeError(const std::string& msg)
    : std::runtime_error(msg)
  {
  }

  void SetMessage(const std::string& msg)
  {
    static_cast<std::runtime_error&>(*this) = std::runtime_error(msg);
  }
};

inline static Handle<Value> VException(const char *msg) {
  HandleScope scope;
  return ThrowException(Exception::Error(String::New(msg)));
}

} // bitcoinjs

#endif
