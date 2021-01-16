#ifndef SHADERS_H
#define SHADERS_H

#include <unordered_map>
#include <stdexcept>
#include <string>

#include "glm/gtc/type_ptr.hpp"

#include "logger.h"
#include "platform_includes.h"
#include "utils.h"

class ShaderProgram {
 public:
  ShaderProgram(const std::string& vertex_shader_file_name,
                const std::string& fragment_shader_file_name);

  void Activate() {
    if (current_program_ != program_) {
      glUseProgram(program_);
      current_program_ = program_;
    }
  }

  // We shouldn't need to check for -1 because glUniform*(-1, ...)
  // is supposed to be silently ignored (no-op). Unfortunately
  // Firefox didn't seem to get the memo, and will return an error.
  void SetUniform(const NameLiteral& name, GLint x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      auto it = uniform_cache_1i_.find(name);
      if (it == uniform_cache_1i_.end() || it->second != x) {
        glUniform1i(location, x);
        uniform_cache_1i_.insert_or_assign(name, x);
      }
    }
  }

  void SetUniform(const NameLiteral& name, GLuint x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform1ui(location, x);
    }
  }

  void SetUniform(const NameLiteral& name, GLfloat x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform1f(location, x);
    }
  }

  void SetUniform(const NameLiteral& name, const glm::vec2& x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform2fv(location, 1, glm::value_ptr(x));
    }
  }

  void SetUniform(const NameLiteral& name, const glm::vec3& x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform3fv(location, 1, glm::value_ptr(x));
    }
  }

  void SetUniform(const NameLiteral& name, const glm::vec4& x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform4fv(location, 1, glm::value_ptr(x));
    }
  }

  void SetUniform(const NameLiteral& name, const glm::mat3& x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(x));
    }
  }

  void SetUniform(const NameLiteral& name, const glm::mat4& x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(x));
    }
  }

  ~ShaderProgram();
 private:
  GLint GetUniformLocation(const NameLiteral& name);

  static GLuint current_program_;

  std::string vertex_shader_file_name_;
  std::string fragment_shader_file_name_;
  GLuint vertex_shader_;
  GLuint fragment_shader_;
  GLuint program_;

  std::unordered_map<NameLiteral, GLint, NameLiteralHash> uniform_locations_cache_;
  std::unordered_map<NameLiteral, GLint, NameLiteralHash> uniform_cache_1i_;
};

// Get a pointer to a ShaderProgram built from the specified
// sources. Reuses existing programs if already exists.
ShaderProgram* GetShader(
  const std::string& vertex_shader_file_name,
  const std::string& fragment_shader_file_name);

#endif // SHADERS_H

