#include <vector>
#include <span>
#include <render/material.h>
#include <render/mesh.h>
#include <render/shader.h>
#include <render/direction_light.h>
#include "debug_bones.h"

static void add_triangle(vec3 a, vec3 b, vec3 c, std::vector<uint> &indices, std::vector<vec3> &vert, std::vector<vec3> &normal)
{
  uint k = vert.size();
  vec3 n = normalize(cross(b - a, c - a));
  indices.push_back(k);
  indices.push_back(k + 2);
  indices.push_back(k + 1);
  vert.push_back(a);
  vert.push_back(b);
  vert.push_back(c);
  normal.push_back(n);
  normal.push_back(n);
  normal.push_back(n);
}

struct DebugBone
{
  MaterialPtr boneMaterial;
  MeshPtr boneMesh;

  std::vector<mat4> instancesTm;
  std::vector<vec4> instancesColor;

  DebugBone()
  {
    boneMaterial = make_material("bone", "sources/shaders/arrow_vs.glsl", "sources/shaders/arrow_ps.glsl");
    std::vector<uint> indices;
    std::vector<vec3> vert;
    std::vector<vec3> normal;
    vec3 c1 = vec3(0, 0, 0);
    vec3 c2 = vec3(0, 1, 0);
    const int N = 4;
    vec3 p[N];
    for (int i = 0; i < N; i++)
    {
      float a1 = ((float)(i) / N) * 2 * PI;
      float a2 = ((float)(i + 1) / N) * 2 * PI;
      vec3 p1 = p[i] = vec3(cos(a1) / 10, 1.0/5, sin(a1) / 10);
      vec3 p2 = vec3(cos(a2) / 10, 1.0/5, sin(a2) / 10);
      add_triangle(p2, p1, c1, indices, vert, normal);
      add_triangle(p2, p1, c2, indices, vert, normal);
    }
    boneMesh = make_mesh(indices, vert, normal);
  }
};

static std::unique_ptr<DebugBone> boneRender;

void create_bone_render()
{
  boneRender = std::make_unique<DebugBone>();
}

static mat4 directionMatrix(vec3 from, vec3 to)
{
  from = normalize(from);
  to = normalize(to);
  quat q(from, to);
  return toMat4(q);
}

static void add_bone(DebugBone &renderer, const vec3 &from, const vec3 &to, vec3 color, float size)
{
  vec3 d = to - from;
  mat4 t = translate(mat4(1.f), from);
  float len = length(d);
  if (len < 0.01f)
    return;
  mat4 s = scale(mat4(1.f), vec3(len, len, len));

  mat4 r = directionMatrix(vec3(0, 1, 0), d);
  renderer.instancesTm.emplace_back(t * r * s);
  renderer.instancesColor.emplace_back(vec4(color, 1.f));
}

void draw_bone(const mat4 &transform, const vec3 &from, const vec3 &to, vec3 color, float size)
{
  if (boneRender)
    add_bone(*boneRender, transform * vec4(from, 1), transform * vec4(to, 1), color, size);
}
void draw_bone(const vec3 &from, const vec3 &to, vec3 color, float size)
{
  if (boneRender)
    add_bone(*boneRender, from, to, color, size);
}

void render_bones(const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  if (!boneRender)
    return;

  DebugBone &renderer = *boneRender;
  if (renderer.instancesTm.empty())
    return;
  const auto &shader = renderer.boneMaterial->get_shader();

  glDepthFunc(GL_ALWAYS);
  glDepthMask(GL_FALSE);
  glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  glLineWidth(2.0f);

  shader.use();

  shader.set_mat4x4("ViewProjection", cameraProjView);
  shader.set_vec3("CameraPosition", cameraPosition);
  shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
  shader.set_vec3("AmbientLight", light.ambient);
  shader.set_vec3("SunLight", light.lightColor);

  const int N = 128;
  int instancesCount = renderer.instancesTm.size();
  for (int i = 0; i < instancesCount; i += N)
  {
    int count = min(N, instancesCount - i);
    shader.set_mat4x4("ArrowTm", std::span(renderer.instancesTm.data() + i, count));
    shader.set_vec4("ArrowColor", std::span(renderer.instancesColor.data() + i, count));
    render(renderer.boneMesh, count);
  }

  glLineWidth(1.0f);
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  renderer.instancesTm.clear();
  renderer.instancesColor.clear();
}
