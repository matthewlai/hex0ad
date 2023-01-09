#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <ostream>
#include <random>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#if defined(_WIN32)
#define _CRT_RAND_S
#include <stdlib.h>
#endif

#include "logger.h"
#include "platform_includes.h"

// SDL uses return values to indicate error. We have these macros to log and
// convert them to exceptions.
#define CHECK_SDL_ERROR(x) \
    do { if (x < 0) { LOG_ERROR("SDL Error: %", SDL_GetError()); \
    throw std::runtime_error(SDL_GetError()); }} while (0);

#define CHECK_SDL_ERROR_PTR(x) \
    do { if (x == nullptr) { LOG_ERROR("SDL Error: %", SDL_GetError()); \
    throw std::runtime_error(SDL_GetError());; }} while (0);

#define CHECK_GL_ERROR \
    do { auto error = glGetError(); if (error != GL_NO_ERROR) { \
      LOG_ERROR("OpenGL error: % (%:%)", gl_error_string(error), __FILE__, __LINE__); abort(); }} while (0);

inline char const* gl_error_string(GLenum const err) noexcept {
  switch (err) {
    // opengl 2 errors (8)
    case GL_NO_ERROR:
      return "GL_NO_ERROR";

    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";

    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";

    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";

    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";

    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";

    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";

    case GL_TABLE_TOO_LARGE:
      return "GL_TABLE_TOO_LARGE";

    // opengl 3 errors (1)
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";

    // gles 2, 3 and gl 4 error are handled by the switch above
    default:
      assert(!"unknown error");
      return nullptr;
  }
}

std::vector<std::uint8_t> ReadWholeFile(const std::string& path);

std::string ReadWholeFileString(const std::string& path);

bool HaveDebugFolder();

void WriteWholeFileString(const std::string& path, const std::string& data);

// Get steady clock time in microseconds (for durations only).
inline uint64_t GetTimeUs() {
  using Clock = std::chrono::steady_clock;
  auto now = Clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

// std::random_device is not implemented on Windows. Not sure about
// emscripten, so we mix in other shit as backup.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85494
inline unsigned int RngSeed() {
  static std::random_device rd;
  unsigned int ret = rd();
  ret ^= std::chrono::high_resolution_clock::now().time_since_epoch().count();

  #if defined(_WIN32)
  // MinGW headers don't seem to provide Microsoft's recommended rand_s(), so we use rand()
  // instead. But we are not seeding it, so what does it do? Who knows!
  ret ^= rand();
  #endif

  #ifdef __EMSCRIPTEN__
  ret ^= static_cast<unsigned int>(emscripten_random() * static_cast<float>(std::numeric_limits<unsigned int>::max()));
  #endif

  return ret;
}

// Convenience function to generate a number in [a, b-1].
inline int64_t Rand(int64_t a, int64_t b) {
  static std::mt19937 rng(RngSeed());
  std::uniform_int_distribution dist(a, b - 1);
  return dist(rng);
}

// This is the djb2 algorithm: http://www.cse.yorku.ca/~oz/hash.html
constexpr std::size_t Djb2(const char* s) {
  std::size_t hash = 5381;
  int c = 0;
  const char* str = s;
  while ((c = *str++)) {
    hash = hash * 33 + c;
  }
  return hash;
}

// This is a wrapper around const char* with hash.
class NameLiteral {
 public:
  constexpr NameLiteral(const char* s) : s_(s), hash_(Djb2(s)) {}

  constexpr const char* Ptr() const { return s_; };

  constexpr bool operator==(const NameLiteral& other) const {
    if (s_ == other.s_) {
      return true;
    } else if (hash_ != other.hash_) {
      return false;
    } else {
      return strcmp(s_, other.s_) == 0;
    }
  }

  constexpr bool operator!=(const NameLiteral& other) const {
    return !(*this == other);
  }

  constexpr std::size_t Hash() const { return hash_; }

 private:
  const char* s_;
  std::size_t hash_;
};

inline std::ostream& operator<<(std::ostream& os, const NameLiteral& name) {
  os << name.Ptr();
  return os;
}

constexpr NameLiteral operator""_name(const char *str, std::size_t len) {
  (void) len;
  return NameLiteral(str);
}

struct NameLiteralHash {
  std::size_t operator()(NameLiteral const& s) const noexcept {
    return s.Hash();
  }
};

using namespace std::string_literals;

struct BoneTransform {
  glm::vec3 translation;
  glm::fquat orientation;

  glm::mat4 ToMatrix() {
    return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(orientation);
  }

  glm::mat4 ToInvMatrix() {
    return glm::toMat4(glm::inverse(orientation)) * glm::translate(glm::mat4(1.0f), -translation);
  }
};

inline BoneTransform ReadBoneTransform(const float* data) {
  BoneTransform bone_transform;
  bone_transform.translation = glm::vec3(data[0], data[1], data[2]);

  // We store in xyvw, and glm wants wxyz.
  bone_transform.orientation = glm::fquat(data[6], data[3], data[4], data[5]);

  return bone_transform;
}

#ifdef HAVE_GLEW
// For OpenGL debugging. This is adapted from https://learnopengl.com/In-Practice/Debugging
void APIENTRY GlDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);
#endif

#endif // UTILS_H
