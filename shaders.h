#ifndef SHADERS_H
#define SHADERS_H

#include <stdexcept>
#include <string>

#include "logger.h"

#include "platform_includes.h"

class ShaderProgram {
 public:
  ShaderProgram(const std::string& vertex_shader_file_name,
                const std::string& fragment_shader_file_name);

  void Activate() { glUseProgram(program_); }

  GLint GetUniformLocation(const std::string& name);

  ~ShaderProgram();
 private:
  std::string vertex_shader_file_name_;
  std::string fragment_shader_file_name_;
  GLuint vertex_shader_;
  GLuint fragment_shader_;
  GLuint program_;
};

// Get a pointer to a ShaderProgram built from the specified
// sources. Reuses existing programs if already exists.
ShaderProgram* GetShader(
  const std::string& vertex_shader_file_name,
  const std::string& fragment_shader_file_name);

#endif // SHADERS_H

