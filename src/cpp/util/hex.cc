#include "hex.h"

namespace bitcoinjs {

char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B','C','D','E','F'};

std::string
Hex::ToHex(const char *data, size_t size)
{
  std::string str;
  for (unsigned int i = 0; i < size; ++i) {
    const char ch = data[i];
    str.append(&hex[(ch  & 0xF0) >> 4], 1);
    str.append(&hex[ch & 0xF], 1);
  }
  return str;
}

std::string
Hex::ToHex(std::string s)
{
  return ToHex(s.c_str(), s.size());
}

std::string
Hex::ToHex(std::vector<char> v)
{
  return ToHex(v.data(), v.size());
}

std::string
Hex::ToHex(uint256_t hash)
{
  return ToHex((char*) &hash, sizeof(uint256_t));
}

} // bitcoinjs
