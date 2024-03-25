#include "material.h"


void Material::bind_uniforms_to_shader() const
{
  const auto &uniforms = shader->uniforms;

  int textureBinding = 0;
  for (const Property &property : properties)
  {

    int location = uniforms[property.shaderUniformIdx].shaderLocation;
    if (const auto *v = std::get_if<float>(&property.value))
      shader->set_float(location, *v);
    else if (const auto *v = std::get_if<glm::vec2>(&property.value))
      shader->set_vec2(location, *v);
    else if (const auto *v = std::get_if<glm::vec3>(&property.value))
      shader->set_vec3(location, *v);
    else if (const auto *v = std::get_if<glm::vec4>(&property.value))
      shader->set_vec4(location, *v);
    else if (const auto *v = std::get_if<Texture2DPtr>(&property.value))
    {
      unsigned textureObject = (*v)->textureObject;
      glActiveTexture(GL_TEXTURE0 + textureBinding);
      glBindTexture(GL_TEXTURE_2D, textureObject);
      glUniform1i(location, textureBinding);
      textureBinding++;
    }
  }
}




