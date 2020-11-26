#include "utils.h"

std::vector<std::uint8_t> ReadWholeFile(const std::string& path) {
  std::ifstream is(path.c_str(), std::ifstream::in | std::ifstream::binary);
  is.seekg(0, is.end);
  auto len = is.tellg();
  is.seekg(0, is.beg);
  std::vector<std::uint8_t> buf(len);
  is.read(reinterpret_cast<char*>(buf.data()), len);
  return buf;
}
