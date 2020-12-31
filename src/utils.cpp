#include "utils.h"

#include <cstring>

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

std::string ReadWholeFileString(const std::string& path) {
  std::ifstream is(path.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!is) {
    throw std::runtime_error(std::string("Opening ") + path + " failed: " + strerror(errno));
  }
  is.seekg(0, is.end);
  auto len = is.tellg();
  is.seekg(0, is.beg);
  std::string buf(len, '\0');
  is.read(reinterpret_cast<char*>(buf.data()), len);
  return buf;
}

#ifdef HAVE_GLEW
void APIENTRY GlDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam) {
  (void) source;
  (void) severity;
  (void) length;
  (void) userParam;

  if (id == 131185) {
    // Buffer creation.
    return;
  }

  std::string log_severity;

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: log_severity = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: log_severity = "med"; break;
    case GL_DEBUG_SEVERITY_LOW: log_severity = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: log_severity = "notify"; break;
  }

  Logger::eLevel level;

  switch (type)
  {
      case GL_DEBUG_TYPE_ERROR: level = Logger::eLevel::ERROR; break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: level = Logger::eLevel::WARN; break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: level = Logger::eLevel::ERROR; break; 
      case GL_DEBUG_TYPE_PORTABILITY: level = Logger::eLevel::ERROR; break;
      case GL_DEBUG_TYPE_PERFORMANCE: level = Logger::eLevel::WARN; break;
      case GL_DEBUG_TYPE_MARKER: level = Logger::eLevel::INFO; break;
      case GL_DEBUG_TYPE_PUSH_GROUP: level = Logger::eLevel::INFO; break;
      case GL_DEBUG_TYPE_POP_GROUP: level = Logger::eLevel::INFO; break;
      case GL_DEBUG_TYPE_OTHER: level = Logger::eLevel::INFO; break;
  }

  logger.Log(level, "[OpenGL] (%, %) %", log_severity, id, message);
}
#endif