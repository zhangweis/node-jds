#ifndef BITCOINJS_SERVER_INCLUDE_UTIL_PARSER_H_
#define BITCOINJS_SERVER_INCLUDE_UTIL_PARSER_H_

#include <string>
#include <stdio.h>

#include "inttypes.h"
#include "slice.h"
#include "exception.h"
#include "stream.h"

namespace bitcoinjs {

class ParserError : public RuntimeError
{
 public:
  ParserError(const std::string& msg, size_t pos, size_t length)
  {
      std::ostringstream s;
      s << msg << " (pos: " << pos << ", length: " << length << ")";
      SetMessage(s.str());
  }
};

/**
 * Parses binary data from a source.
 */
class Parser {
 public:
  Parser(const uint8_t *data, size_t length)
    : data(data), pos(0), length(length), is(NULL) {}
  Parser(const char *data, size_t length)
    : data(reinterpret_cast<const uint8_t*>(data)), pos(0), length(length), is(NULL) {}
  Parser(const std::vector<char> &v)
    : data(NULL), pos(0), length(0), is(NULL) {
    length = v.size();
    if (length > 0) {
      data = reinterpret_cast<const uint8_t*>(&v[0]);
    }
  }
  Parser(std::istream *stream) : data(NULL), pos(0), length(0), is(stream) {}

  /**
   * Prepare data for reading.
   *
   * This method will guarantee that this number of bytes is
   * available in the internal buffer for reading or throw an
   * exception.
   */
  void Align(int offset)
  {
    // If enough data is available we're done
    if ((pos + offset) <= length) return;

    // Can we get more data from an input stream?
    if (is != NULL && !is->eof()) {
      uint32_t avail = length - pos;

      // If we have bytes left in our buffer, move those to the beginning
      if (avail > 0 && ibuf.data() != data) {
        memmove(ibuf.data(), data, avail);
      }

      // Get the remainder of the requested data from the input stream
      ibuf.resize(offset);
      data = ibuf.data();
      is->read(reinterpret_cast<char*>(ibuf.data() + avail), offset - avail);

      // Fail if there wasn't enough data
      if (is->eof()) {
        throw ParserError("Parser::Align(): EOF", pos + offset, length);
      }

      length = offset;
      pos = 0;
    } else {
      throw ParserError("Parser::Align(): Out of bounds", pos + offset, length);
    }
  }

  inline void Seek(int offset)
  {
    if ((pos + offset) <= length) {
      pos += offset;
    } else if (is != NULL && !is->eof()) {
      offset -= length - pos;
      pos = length;

      is->seekg(offset, std::ios::cur);

      if (is->fail()) {
        throw ParserError("Parser::Seek(): Input stream seek failed", pos + offset, length);
      }
    } else {
      throw ParserError("Parser::Seek(): Out of bounds", pos + offset, length);
    }
  }

  inline size_t Tell() const { return pos; }

  inline const uint8_t* Data(size_t size)
  {
    if ((pos + size) > length) {
      Align(size);
    }
    pos += size;
    return (data + pos - size);
  }

  uint8_t Uint8()
  {
    uint8_t result = *Data(1);
    return result;
  }

  uint16_t Uint16()
  {
    if (!IntTool::IsLittleEndian16()) {
      return IntTool::Swap(*reinterpret_cast<const uint16_t*>(Data(2)));
    } else {
      return *reinterpret_cast<const uint16_t*>(Data(2));
    }
  }

  uint32_t Uint32()
  {
    if (!IntTool::IsLittleEndian32()) {
      return IntTool::Swap(*reinterpret_cast<const uint32_t*>(Data(4)));
    } else {
      return *reinterpret_cast<const uint32_t*>(Data(4));
    }
  }

  uint64_t Uint64()
  {
    if (!IntTool::IsLittleEndian64()) {
      return IntTool::Swap(*reinterpret_cast<const uint64_t*>(Data(8)));
    } else {
      return *reinterpret_cast<const uint64_t*>(Data(8));
    }
  }

  uint256_t Uint256()
  {
    uint256_t out;

    memcpy(out.d8, Data(32), 32);

    return out;
  }

  std::string Str(size_t len)
  {
    return std::string(reinterpret_cast<const char*>(Data(len)), len);
  }

  void Read(uint8_t *obuf, size_t len)
  {
    memcpy(obuf, Data(len), len);
  }

  /**
   * Parser can be extended with user-defined types.
   *
   * You can either instantiate this template directly or by using the
   * Serializable() interface.
   */
  template<typename T>
  T Parse();

 private:
  const uint8_t *data;
  size_t pos;
  size_t length;
  std::istream *is;
  std::vector<uint8_t> ibuf;
};

} // bitcoinjs

#endif
