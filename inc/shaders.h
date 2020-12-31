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

  void SetUniform(const NameLiteral& name, GLint x) {
    glUniform1i(GetUniformLocation(name), x);
  }

  void SetUniform(const NameLiteral& name, GLuint x) {
    glUniform1ui(GetUniformLocation(name), x);
  }

  void SetUniform(const NameLiteral& name, GLfloat x) {
    glUniform1f(GetUniformLocation(name), x);
  }

  void SetUniform(const NameLiteral& name, const glm::vec2& x) {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(x));
  }

  void SetUniform(const NameLiteral& name, const glm::vec3& x) {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(x));
  }

  void SetUniform(const NameLiteral& name, const glm::vec4& x) {
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(x));
  }

  void SetUniform(const NameLiteral& name, const glm::mat3& x) {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(x));
  }

  void SetUniform(const NameLiteral& name, const glm::mat4& x) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(x));
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

