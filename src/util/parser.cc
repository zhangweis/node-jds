#include "parser.h"

#include <cstdio>

void Parser::Seek(int offset)
{
  // TODO: Check bounds
  pos += offset;
}

uint64_t Parser::VarInt()
{
  uint64_t result = 0;
  if (data[pos] < 0xFD) {
    result = data[pos++];
  } else if (data[pos] == 0xFD) {
    result = * (uint16_t*) (data+pos+1);
    pos += 3;
  } else if (data[pos] == 0xFE) {
    result = * (uint32_t*) (data+pos+1);
    pos += 5;
  } else { // must be 0xFF
    result = * (uint64_t*) (data+pos+1);
    pos += 9;
  }
  if (result > VARINT_MAX) {
    // TODO: Error
  }
  return result;
}
