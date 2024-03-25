#pragma once
#include <map>
#include <memory>


struct Mesh
{
  const uint32_t vertexArrayBufferObject;
  const int numIndices;

  Mesh(uint32_t vertexArrayBufferObject, int numIndices) :
    vertexArrayBufferObject(vertexArrayBufferObject),
    numIndices(numIndices)
    {}
};

using MeshPtr = std::shared_ptr<Mesh>;

MeshPtr load_mesh(const char *path, int idx);
MeshPtr make_plane_mesh();

void render(const MeshPtr &mesh);