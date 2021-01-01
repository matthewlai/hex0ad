#ifndef SHADERS_H
#define SHADERS_H

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

  void Activate() { glUseProgram(program_); }

  // We shouldn't need to check for -1 because glUniform*(-1, ...)
  // is supposed to be silently ignored (no-op). Unfortunately
  // Firefox didn't seem to get the memo, and will return an error.
  void SetUniform(const NameLiteral& name, GLint x) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
      glUniform1i(location, x);
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

  std::string vertex_shader_file_name_;
  std::string fragment_shader_file_name_;
  GLuint vertex_shader_;
  GLuint fragment_shader_;
  GLuint program_;

  LinearCache<NameLiteral, GLint> uniform_locations_cache_;
};

// Get a pointer to a ShaderProgram built from the specified
// sources. Reuses existing programs if already exists.
ShaderProgram* GetShader(
  const std::string& vertex_shader_file_name,
  const std::string& fragment_shader_file_name);

#endif // SHADERS_H

