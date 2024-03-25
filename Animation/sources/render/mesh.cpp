#include "mesh.h"
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>
#include "glad/glad.h"


static void create_indices(const std::vector<unsigned int> &indices)
{
  GLuint arrayIndexBuffer;
  glGenBuffers(1, &arrayIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);
}

static void init_channel(int index, size_t data_size, const void *data_ptr, int component_count, bool is_float)
{
  GLuint arrayBuffer;
  glGenBuffers(1, &arrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  glBufferData(GL_ARRAY_BUFFER, data_size, data_ptr, GL_STATIC_DRAW);
  glEnableVertexAttribArray(index);

  if (is_float)
    glVertexAttribPointer(index, component_count, GL_FLOAT, GL_FALSE, 0, 0);
  else
    glVertexAttribIPointer(index, component_count, GL_UNSIGNED_INT, 0, 0);
}


template<int i>
static void InitChannel() { }

template<int i, typename T, typename... Channel>
static void InitChannel(const std::vector<T> &channel, const Channel&... channels)
{
  if (channel.size() > 0)
  {
    const int size = sizeof(T) / sizeof(channel[0][0]);
    init_channel(i, sizeof(T) * channel.size(), channel.data(), size, !(std::is_same<T, uvec4>::value));
  }
  InitChannel<i + 1>(channels...);
}


template<typename... Channel>
MeshPtr create_mesh(const std::vector<unsigned int> &indices, const Channel&... channels)
{
  uint32_t vertexArrayBufferObject;
  glGenVertexArrays(1, &vertexArrayBufferObject);
  glBindVertexArray(vertexArrayBufferObject);
  InitChannel<0>(channels...);
  create_indices(indices);
  return std::make_shared<Mesh>(vertexArrayBufferObject, indices.size());
}


MeshPtr create_mesh(const aiMesh *mesh)
{
  std::vector<uint32_t> indices;
  std::vector<vec3> vertices;
  std::vector<vec3> normals;
  std::vector<vec2> uv;
  std::vector<vec4> weights;
  std::vector<uvec4> weightsIndex;

  int numVert = mesh->mNumVertices;
  int numFaces = mesh->mNumFaces;

  if (mesh->HasFaces())
  {
    indices.resize(numFaces * 3);
    for (int i = 0; i < numFaces; i++)
    {
      assert(mesh->mFaces[i].mNumIndices == 3);
      for (int j = 0; j < 3; j++)
        indices[i * 3 + j] = mesh->mFaces[i].mIndices[j];
    }
  }

  if (mesh->HasPositions())
  {
    vertices.resize(numVert);
    for (int i = 0; i < numVert; i++)
      vertices[i] = to_vec3(mesh->mVertices[i]);
  }

  if (mesh->HasNormals())
  {
    normals.resize(numVert);
    for (int i = 0; i < numVert; i++)
      normals[i] = to_vec3(mesh->mNormals[i]);
  }

  if (mesh->HasTextureCoords(0))
  {
    uv.resize(numVert);
    for (int i = 0; i < numVert; i++)
      uv[i] = to_vec2(mesh->mTextureCoords[0][i]);
  }

  if (mesh->HasBones())
  {
    weights.resize(numVert, vec4(0.f));
    weightsIndex.resize(numVert);
    int numBones = mesh->mNumBones;
    std::vector<int> weightsOffset(numVert, 0);
    for (int i = 0; i < numBones; i++)
    {
      const aiBone *bone = mesh->mBones[i];
      //bonesMap[std::string(bone->mName.C_Str())] = i;

      for (unsigned j = 0; j < bone->mNumWeights; j++)
      {
        int vertex = bone->mWeights[j].mVertexId;
        int offset = weightsOffset[vertex]++;
        weights[vertex][offset] = bone->mWeights[j].mWeight;
        weightsIndex[vertex][offset] = i;
      }
    }
    //the sum of weights not 1
    for (int i = 0; i < numVert; i++)
    {
      vec4 w = weights[i];
      float s = w.x + w.y + w.z + w.w;
      weights[i] *= 1.f / s;
    }
  }
  return create_mesh(indices, vertices, normals, uv, weights, weightsIndex);
}

MeshPtr load_mesh(const char *path, int idx)
{

  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);

  importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_LimitBoneWeights |
    aiPostProcessSteps::aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder);

  const aiScene* scene = importer.GetScene();
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return nullptr;
  }

  return create_mesh(scene->mMeshes[idx]);
}

void render(const MeshPtr &mesh)
{
  glBindVertexArray(mesh->vertexArrayBufferObject);
  glDrawElementsBaseVertex(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0, 0);
}

MeshPtr make_plane_mesh()
{
  std::vector<uint32_t> indices = {0,1,2,0,2,3};
  std::vector<vec3> vertices = {vec3(-1,0,-1), vec3(1,0,-1), vec3(1,0,1), vec3(-1,0,1)};
  std::vector<vec3> normals(4, vec3(0,1,0));
  std::vector<vec2> uv = {vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1)};
  return create_mesh(indices, vertices, normals, uv);
}
