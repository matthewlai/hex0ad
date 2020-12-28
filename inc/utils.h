#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "logger.h"

// SDL uses return values to indicate error. We have these macros to log and
// convert them to exceptions.
#define CHECK_SDL_ERROR(x) \
    do { if (x < 0) { LOG_ERROR("SDL Error: %", SDL_GetError()); \
    throw std::runtime_error(SDL_GetError()); }} while (0);

#define CHECK_SDL_ERROR_PTR(x) \
    do { if (x == nullptr) { LOG_ERROR("SDL Error: %", SDL_GetError()); \
    throw std::runtime_error(SDL_GetError());; }} while (0);

#define CHECK_GL_ERROR \
    do { if (glGetError() != GL_NO_ERROR) { \
      throw std::runtime_error(std::string("OpenGL error: ") + \
        std::to_string(glGetError())); } } while (0);

std::vector<std::uint8_t> ReadWholeFile(const std::string& path);

// Get steady clock time in microseconds (for durations only).
inline uint64_t GetTimeUs() {
  using Clock = std::chrono::steady_clock;
  auto now = Clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

// We use pointer lookup for caches where we know the name will
// (should) always be a literal. We define a literal wrapper
// type to ensure that.
class NameLiteral {
 public:
  constexpr NameLiteral(const char* s) : s_(s) {}
  constexpr const char* Ptr() const { return s_; };
  constexpr bool operator==(const NameLiteral& other) const {
    // Pointer comparison is intentional. This is what makes
    // it fast.
    return s_ == other.s_;
  }

  constexpr bool operator!=(const NameLiteral& other) const {
    return !(*this == other);
  }

  constexpr bool operator<(const NameLiteral& other) const {
    return s_ < other.s_;
  }

 private:
  const char* s_;
};

inline std::ostream& operator<<(std::ostream& os, const NameLiteral& name) {
  os << name.Ptr();
  return os;
}

constexpr NameLiteral operator""_name(const char *str, std::size_t len) {
  (void) len;
  return NameLiteral(str);
}

// Linear cache for very small tables (eg. uniform locations) where
// either BST or hashing would be slower, and we want good cache
// locality. It's insert-only, and only lookup speed is prioritized.
template <typename K, typename V>
class LinearCache {
 public:
  // Warn if the table gets to this big.
  // This is around when we should be switching to binary search.
  // https://dirtyhandscoding.wordpress.com/2017/08/25/performance-comparison-linear-search-vs-binary-search/
  const static std::size_t kSizeToWarn = 512;

  void InsertOrReplace(const K& k, const V& v) {
    V* existing_ptr = Find(k);

    if (existing_ptr != nullptr) {
      *existing_ptr = v;
    } else {
      keys_.push_back(k);
      values_.push_back(v);
    }

    if (keys_.size() > kSizeToWarn) {
      LOG_WARN("LinearCache is now at size %. Maybe switch to BST?",
               keys_.size());
    }
  }

  // Lookup. Returns pointer to found element or nullptr.
  V* Find(const K& k) {
    for (std::size_t i = 0; i < keys_.size(); ++i) {
      if (k == keys_[i]) {
        return &values_[i];
      }
    }
    return nullptr;
  }

 private:
  // Separate keys and value vectors for cache locality.
  std::vector<K> keys_;
  std::vector<V> values_;
};

#endif // UTILS_H
