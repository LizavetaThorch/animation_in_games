#include "shader.h"
#include <iostream>
#include <map>
#include "log.h"
#include "glad/glad.h"
#include <filesystem>
#include <array>
#include <vector>
#include <fstream>


static void read_shader_info(Shader &shader)
{
  GLuint program = shader.program;

  int count;
  const GLsizei bufSize = 128;
  GLchar name[bufSize];
  GLsizei length;
  shader.uniforms.clear();
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
  for (int i = 0; i < count; i++)
  {
    GLenum type;
    GLint size;
    glGetActiveUniform(program, (GLuint)i, bufSize, &length, &size, &type, name);
    //debug_log("uniform %s #%d Type: %u Name: %s", shader.name.c_str(), i, type, name);

    GLint shaderLocation = glGetUniformLocation(program, name);
    shader.uniforms.emplace_back(ShaderUniform{std::string(name), type, shaderLocation});
  }
}

struct ShaderInfo
{
  GLenum shaderType;
  std::string path;
  std::string sources;
};

static bool compile_shader(const char *shaderName, const std::vector<ShaderInfo> &shaders, GLuint &program)
{
  std::vector<GLuint> compiled_shaders;
  compiled_shaders.reserve(shaders.size());
  GLchar infoLog[1024];
  GLint success;
  for (const ShaderInfo &shader : shaders)
  {
    GLuint shaderProg = glCreateShader(shader.shaderType);
    const GLchar * shaderCode = shader.sources.c_str();
    glShaderSource(shaderProg, 1, &shaderCode, NULL);
    glCompileShader(shaderProg);
    glGetShaderiv(shaderProg, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      glGetShaderInfoLog(shaderProg, 512, NULL, infoLog);
      debug_error("Shader (%s) compilation failed!\n Log: %s", shader.path.c_str(), infoLog);
      return false;
    };
    compiled_shaders.push_back(shaderProg);
  }


  program = glCreateProgram();
  for (GLuint shaderProg : compiled_shaders)
    glAttachShader(program, shaderProg);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success)
  {
    glGetProgramInfoLog(program, 1024, NULL, infoLog);
    debug_error("Shader programm (%s) linking failed!\n Log: %s", shaderName, infoLog);
    return false;
  }

  for (GLuint shaderProg : compiled_shaders)
    glDeleteShader(shaderProg);
  return true;
}



static std::string read_file(const char *path)
{
  std::ifstream file(path);
  return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static bool compile_shader(const char *name, const Shader::ShaderSources &sources, GLuint &program)
{
  std::vector<ShaderInfo> shaderCode;

  for (const auto &[shaderType, path] : sources)
  {
    shaderCode.emplace_back(ShaderInfo{shaderType, path, read_file(path.c_str())});
  }
  return compile_shader(name, shaderCode, program);
}

static std::vector<ShaderPtr> shaderList;

ShaderPtr compile_shader(const char *name, const char *vs_path, const char *ps_path)
{
  Shader::ShaderSources shaderSources{{GL_VERTEX_SHADER, vs_path}, {GL_FRAGMENT_SHADER, ps_path}};

  GLuint program;
  if (compile_shader(name, shaderSources, program))
  {
    auto shader = std::make_shared<Shader>(name, program, shaderSources);
    read_shader_info(*shader);
    shaderList.push_back(shader);
    return shader;
  }
  return nullptr;
}


void recompile_all_shaders()
{
  for (auto &shader : shaderList)
  {
    GLuint program;
    if (compile_shader(shader->name.c_str(), shader->shaderSources, program))
    {
      glDeleteProgram(shader->program);
      shader->program = program;
      read_shader_info(*shader);
    }
  }
}