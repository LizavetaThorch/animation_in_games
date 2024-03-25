#pragma once

#include <memory>

struct Texture2D
{
  const unsigned textureObject;
  Texture2D(unsigned textureObject) : textureObject(textureObject) {}
};

using Texture2DPtr = std::shared_ptr<Texture2D>;

Texture2DPtr create_texture2d(const char *path);