#include "shaders.h"

#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#include "logger.h"
#include "utils.h"

#include "platform_includes.h"

namespace {
struct ShaderCompileResult {
  GLuint shader = 0;
  bool success = false;
  std::string log;
  std::string processed_source;
};

void PrintSourceWithLineNumbers(const std::string& source) {
  std::stringstream ss(source);
  std::stringstream ss_out;
  std::string line;
  int line_number = 1;
  while (std::getline(ss, line)) {
    ss_out << line_number << '\t' << line + "\n";
    ++line_number;
  }
  LOG_ERROR("Processed source:\n%", ss_out.str());
}

std::string Replace(const std::string& s, const std::string& search,
                    const std::string& replace) {
  std::string::size_type pos = 0;
  std::string ret = s;
  while ((pos = ret.find(search, pos)) != std::string::npos) {
    ret.replace(pos, search.size(), replace);
    pos += replace.size();
  }
  return ret;
}

ShaderCompileResult CompileShader(GLenum shader_type, 
                                  const std::string& source) {
  ShaderCompileResult result;
  result.success = false;

  result.shader = glCreateShader(shader_type);
  if (result.shader == 0) {
    result.log = "Failed to create shader. Invalid type?";
    return result;
  }

  std::string source_proc = source;

  std::map<std::string, std::string> replacements;

  #ifdef USE_OPENGL
  // GLSL ES 3.0 is based on GLSL 3.3.
  replacements["#version 300 es"] = "#version 330";
  #endif

  const char* kIncludePattern = R"""(#include\s"(.+)")""";
  std::regex include_regex(kIncludePattern);

  auto includes_begin = std::sregex_iterator(
      source_proc.begin(), source_proc.end(), include_regex);
  for (auto match = includes_begin; match != std::sregex_iterator(); ++match) {
    replacements[match->str()] = ReadWholeFileString("shaders/"s + match->str(1));
  }

  for (const auto& [find, replace] : replacements) {
    source_proc = Replace(source_proc, find, replace);
  }

  result.processed_source = source_proc;

  const char* source_cstr = source_proc.c_str();
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

/*static*/ GLuint ShaderProgram::current_program_ = 0;

ShaderProgram::ShaderProgram(const std::string& vertex_shader_file_name,
                             const std::string& fragment_shader_file_name) 
  : vertex_shader_file_name_(vertex_shader_file_name),
    fragment_shader_file_name_(fragment_shader_file_name) {
  std::ifstream vertex_file(vertex_shader_file_name);
  std::ifstream fragment_file(fragment_shader_file_name);

  if (!vertex_file) {
    LOG_ERROR("Failed to open vertex shader file: %", vertex_shader_file_name);
    throw std::runtime_error("Failed to open vertex shader file: "s
                             + vertex_shader_file_name);
  }

  if (!fragment_file) {
    LOG_ERROR("Failed to open fragment shader file: %",
              fragment_shader_file_name);
    throw std::runtime_error(
        "Failed to open fragment shader file: "s +
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
    glAttachShader(program_, compile_result.shader);
    vertex_shader_ = compile_result.shader;
  } else {
    LOG_ERROR("Compilation failed:\n%", compile_result.log);
    PrintSourceWithLineNumbers(compile_result.processed_source);
    throw std::runtime_error(
        "Shader compilation failed: "s + compile_result.log);
  }

  LOG_INFO("Compiling % (fragment shader)", fragment_shader_file_name);
  compile_result = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

  if (compile_result.success) {
    glAttachShader(program_, compile_result.shader);
    fragment_shader_ = compile_result.shader;
  } else {
    LOG_ERROR("Compilation failed:\n%", compile_result.log);
    PrintSourceWithLineNumbers(compile_result.processed_source);
    throw std::runtime_error(
        "Shader compilation failed: "s + compile_result.log);
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
          "Shader program linking failed: "s + link_log);
    }
  }
}

GLint ShaderProgram::GetUniformLocation(const NameLiteral& name) {
  auto it = uniform_locations_cache_.find(name);

  if (it == uniform_locations_cache_.end()) {
    auto loc = glGetUniformLocation(program_, name.Ptr());

    constexpr bool kCheckShaderActive = false;
    if (kCheckShaderActive && loc == -1) {
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

    uniform_locations_cache_.insert({name, loc});
    return loc;
  } else {
    return it->second;
  }
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

ShaderProgram* GetShader(
  const std::string& vertex_shader_file_name,
  const std::string& fragment_shader_file_name) {
  std::string lookup_key =
    vertex_shader_file_name + ";" + fragment_shader_file_name;
  static std::map<std::string, ShaderProgram*> shader_cache;
  auto it = shader_cache.find(lookup_key);
  if (it != shader_cache.end()) {
    return it->second;
  } else {
    ShaderProgram* program = new ShaderProgram(vertex_shader_file_name,
                                               fragment_shader_file_name);
    auto insert_result = shader_cache.insert(std::make_pair(lookup_key, program));
    return insert_result.first->second;
  }
}