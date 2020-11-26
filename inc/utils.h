#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <string>
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

#endif // UTILS_H
