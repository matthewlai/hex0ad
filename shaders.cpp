#include "shaders.h"

#include <fstream>
#include <string>

#include "logger.h"

namespace {
struct ShaderCompileResult {
  GLuint shader;
  bool success;
  std::string log;
};

ShaderCompileResult CompileShader(GLenum shader_type, 
                                  const std::string& source) {
  ShaderCompileResult result;
  result.success = false;

  result.shader = glCreateShader(shader_type);
  if (result.shader == 0) {
    result.log = "Failed to create shader. Invalid type?";
    return result;
  }

  const char* source_cstr = source.c_str();
  glShaderSource(result.shader, 1, &source_cstr, nullptr);
  glCompileShader(result.shader);
  
  GLint compiled;
  glGetShaderiv(result.shader, GL_COMPILE_STATUS, &compiled);
  
  if (!compiled) {
    GLint shader_log_len;
    glGetShaderiv(result.shader, GL_INFO_LOG_LENGTH, &shader_log_len);
    result.log.resize(shader_log_len);
    glGetShaderInfoLog(result.shader, shader_log_len, nullptr,
                       &result.log[0]);
    glDeleteShader(result.shader);
    result.shader = 0;
    return result;
  }
  
  result.success = true;
  return result;
}
} // namespace

ShaderProgram::ShaderProgram(const std::string& vertex_shader_file_name,
                             const std::string& fragment_shader_file_name) 
  : vertex_shader_file_name_(vertex_shader_file_name),
    fragment_shader_file_name_(fragment_shader_file_name) {
  std::ifstream vertex_file(vertex_shader_file_name);
  std::ifstream fragment_file(fragment_shader_file_name);

  if (!vertex_file) {
    LOG_ERROR("Failed to open vertex shader file: %", vertex_shader_file_name);
    throw std::runtime_error(std::string("Failed to open vertex shader file: ")
                             + vertex_shader_file_name);
  }

  if (!fragment_file) {
    LOG_ERROR("Failed to open fragment shader file: %",
              fragment_shader_file_name);
    throw std::runtime_error(
        std::string("Failed to open fragment shader file: ") +
        vertex_shader_file_name);
  }

  std::string vertex_shader_source;
  std::string fragment_shader_source;

  std::string line;
  while (std::getline(vertex_file, line)) {
    vertex_shader_source = vertex_shader_source + line + "\n";
  }

  while (std::getline(fragment_file, line)) {
    fragment_shader_source = fragment_shader_source + line + "\n";
  }

  program_ = glCreateProgram();
  if (program_ == 0) {
    LOG_ERROR("Failed to create program object.");
    throw std::runtime_error("Failed to create program object.");
  }

  LOG_INFO("Compiling % (vertex shader)", vertex_shader_file_name);
  auto compile_result = CompileShader(GL_VERTEX_SHADER,
                                      vertex_shader_source);
  if (compile_result.success) {
    LOG_INFO("Success!");
    glAttachShader(program_, compile_result.shader);
    vertex_shader_ = compile_result.shader;
  } else {
    LOG_ERROR("Compilation failed: %", compile_result.log);
    throw std::runtime_error(
        std::string("Shader compilation failed: ") + compile_result.log);
  }

  LOG_INFO("Compiling % (fragment shader)", fragment_shader_file_name);
  compile_result = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

  if (compile_result.success) {
    LOG_INFO("Success!");
    glAttachShader(program_, compile_result.shader);
    fragment_shader_ = compile_result.shader;
  } else {
    LOG_ERROR("Compilation failed: %", compile_result.log);
    throw std::runtime_error(
        std::string("Shader compilation failed: ") + compile_result.log);
  }

  glLinkProgram(program_);
  GLint linked;
  glGetProgramiv(program_, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint log_length;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &log_length);

    if (log_length > 1) {
      std::string link_log(log_length, '\0');
      glGetProgramInfoLog(program_, log_length, nullptr, &link_log[0]);
      LOG_ERROR("Shader program linking failed: %", link_log);
      throw std::runtime_error(
          std::string("Shader program linking failed: ") + link_log);
    }
  }
}

GLint ShaderProgram::GetUniformLocation(const std::string& name) {
  auto loc = glGetUniformLocation(program_, name.c_str());

  if (loc == -1) {
    LOG_ERROR("% is not an active uniform in (%, %)", name,
              vertex_shader_file_name_, fragment_shader_file_name_);
    throw std::runtime_error("GetUniformLocation failed.");
  }

  if (loc == GL_INVALID_VALUE) {
    LOG_ERROR("Invalid shader program (%, %)",
              vertex_shader_file_name_, fragment_shader_file_name_);
    throw std::runtime_error("GetUniformLocation failed.");
  }

  if (loc == GL_INVALID_OPERATION) {
    LOG_ERROR("Shader program has not been successfully linked (%, %)",
              vertex_shader_file_name_, fragment_shader_file_name_);
    throw std::runtime_error("GetUniformLocation failed.");
  }

  return loc;
}

ShaderProgram::~ShaderProgram() {
  if (program_ != 0) {
    if (vertex_shader_ != 0) {
      glDetachShader(program_, vertex_shader_);
      glDeleteShader(vertex_shader_);
      vertex_shader_ = 0;
    }

    if (fragment_shader_ != 0) {
      glDetachShader(program_, fragment_shader_);
      glDeleteShader(fragment_shader_);
      fragment_shader_ = 0;
    }

    glDeleteProgram(program_);
    program_ = 0;
  }
}

