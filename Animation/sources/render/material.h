#pragma once
#include <map>
#include <variant>
#include "log.h"
#include "shader.h"
#include "texture2d.h"

#define TYPES \
  TYPE(float, GL_FLOAT) TYPE(vec2, GL_FLOAT_VEC2) TYPE(vec3, GL_FLOAT_VEC3) TYPE(vec4, GL_FLOAT_VEC4) TYPE(Texture2DPtr, GL_SAMPLER_2D)\


class Material
{
private:
  ShaderPtr shader;
  using MaterialProperty = std::variant<float, glm::vec2, glm::vec3, glm::vec4, Texture2DPtr>;

  struct Property
  {
    std::string name;
    int shaderUniformIdx;
    MaterialProperty value;
  };
  std::vector<Property> properties;

public:

  Material(ShaderPtr &&shader) : shader(std::move(shader)) {}

  const Shader &get_shader() const { return *shader; }
  void bind_uniforms_to_shader() const;

  template<typename T>
  bool set_property(const char *name, T &&value)
  {
    for (Property &p : properties)
    {
      if (p.name == name)
      {
        p.value = std::move(value);
        return true;
      }
    }

    int i = 0;
    for (const ShaderUniform &sampler : shader->uniforms)
    {
      if (sampler.name == name)
      {
        properties.emplace_back(Property{std::string(name), i, MaterialProperty{std::move(value)}});
        return true;
      }
      i++;
    }
    debug_error("property %s in shader %s didn't found", name, shader->name.c_str());
    return false;
  }
};

using MaterialPtr = std::shared_ptr<Material>;

inline MaterialPtr make_material(const char *name, const char *vs_file, const char *ps_file)
{
  ShaderPtr shader = compile_shader(name, vs_file, ps_file);
  return shader ? std::make_shared<Material>(std::move(shader)) : nullptr;
}