#include "utils.h"

std::vector<std::uint8_t> ReadWholeFile(const std::string& path) {
  std::ifstream is(path.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!is) {
    throw std::runtime_error(std::string("Opening ") + path + " failed: " + strerror(errno));
  }
  is.seekg(0, is.end);
  auto len = is.tellg();
  is.seekg(0, is.beg);
  std::vector<std::uint8_t> buf(len);
  is.read(reinterpret_cast<char*>(buf.data()), len);
  return buf;
}
