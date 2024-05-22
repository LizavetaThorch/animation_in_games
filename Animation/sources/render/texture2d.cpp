#include "texture2d.h"
#include "glad/glad.h"
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture2DPtr create_texture(const unsigned char *image, int w, int h, int ch)
{
  GLuint textureObject;
  glGenTextures(1, &textureObject);
  auto texture = std::make_shared<Texture2D>(textureObject);
  GLuint textureType = GL_TEXTURE_2D;

  glBindTexture(textureType, textureObject);

  if (ch == 4)
    glTexImage2D(textureType, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  else if (ch == 3)
    glTexImage2D(textureType, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

  const bool generateMips = true;
  if (generateMips)
  {
    glGenerateMipmap(textureType);
    GLenum mipmapMinPixelFormat = GL_LINEAR_MIPMAP_LINEAR;
    GLenum mipmapMagPixelFormat = GL_LINEAR;

    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, mipmapMinPixelFormat);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, mipmapMagPixelFormat);
  }
  else
  {
    GLenum minMagixelFormat = GL_LINEAR;
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, minMagixelFormat);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, minMagixelFormat);
  }
  glBindTexture(textureType, 0);

  return texture;
}

Texture2DPtr create_texture2d(const char *path)
{
  int w, h, ch;
  stbi_set_flip_vertically_on_load(true);
  auto stbiData = stbi_load(path, &w, &h, &ch, 0);
  // assert(ch == 4);
  Texture2DPtr result;
  if (stbiData)
  {
    result = create_texture(stbiData, w, h, ch);
    stbi_image_free(stbiData);
  }
  return result;
}
