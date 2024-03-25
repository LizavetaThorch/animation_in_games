/* #include <vector>
#include "debug_arrow.h"
#include "shader/shader.h"
#include "resources/resources.h"

void draw_arrow(const mat4 &transform, const vec3 &from, const vec3 &to, vec3 color, float size, bool depth_ignore)
{
  ecs::get_singleton<DebugArrow>()->add_arrow(transform * vec4(from, 1), transform * vec4(to, 1), color, size, depth_ignore);
}
void draw_arrow(const vec3 &from, const vec3 &to, vec3 color, float size, bool depth_ignore)
{
  ecs::get_singleton<DebugArrow>()->add_arrow(from, to, color, size, depth_ignore);
}
void draw_transform(const Transform &transform)
{
  vec3 p = transform.get_position();
  mat3 rotation = transform.get_rotation();
  for (int i = 0; i < 3; i++)
  {
    vec3 n(0.f);
    n[i] = 1.f;
    draw_arrow(p, p + rotation * n * 0.3f, n, 0.03f, false);
  }
}
void DebugArrow::add_triangle(vec3 a, vec3 b, vec3 c, std::vector<uint> &indices, std::vector<vec3> &vert, std::vector<vec3> &normal)
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
DebugArrow::DebugArrow()
{
  arrowMaterial = Material("arrows");
  std::vector<uint> indices;
  std::vector<vec3> vert;
  std::vector<vec3> normal;
  vec3 c = vec3(0, 1, 0);
  const int N = 4;
  vec3 p[N];
  for (int i = 0; i < N; i++)
  {
    float a1 = ((float)(i) / N) * 2 * PI;
    float a2 = ((float)(i + 1) / N) * 2 * PI;
    vec3 p1 = p[i] = vec3(cos(a1), 0, sin(a1));
    vec3 p2 = vec3(cos(a2), 0, sin(a2));
    add_triangle(p2, p1, c, indices, vert, normal);
  }

  add_triangle(p[0], p[1], p[2], indices, vert, normal);
  add_triangle(p[0], p[2], p[3], indices, vert, normal);
  arrow = Mesh(indices, vert, normal);
}

mat4 directionMatrix(vec3 from, vec3 to)
{

  from = normalize(from);
  to = normalize(to);
  quat q(from, to);
  return toMat4(q);
}
void DebugArrow::add_arrow(const vec3 &from, const vec3 &to, vec3 color, float size, bool depth_ignore)
{
  vec3 d = to - from;
  mat4 t = translate(mat4(1.f), from);
  float len = length(d);
  if (len < 0.01f)
    return;
  mat4 s = scale(mat4(1.f), vec3(size, len, size));

  mat4 r = directionMatrix(vec3(0, 1, 0), d);
  if (depth_ignore)
    depthIgnore.emplace_back(Arrow{t * r * s, vec4(color, 1.f)});
  else
    depthNotIgnore.emplace_back(Arrow{t * r * s, vec4(color, 1.f)});
}
void DebugArrow::render_depth_case(UniformBuffer &instanceData, std::vector<Arrow> &arrows, bool ignoreDepth, bool wire_frame)
{
  uint instanceCount = arrows.size(), instanceSize = 0;
  if (instanceCount == 0)
    return;
  char *data = instanceData.get_buffer(0, (instanceCount + 1) * instanceSize);
  // i don't know why need to +1, but it din't work without it
  memcpy(data, arrows.data(), instanceCount * instanceSize);
  instanceData.flush_buffer(instanceCount * instanceSize);

  glDepthFunc(ignoreDepth ? GL_ALWAYS : GL_LESS);
  glDepthMask(ignoreDepth ? GL_FALSE : GL_TRUE);
  arrow.render_instances(instanceCount, wire_frame);
}
void DebugArrow::render(UniformBuffer &instanceData, bool wire_frame)
{
  if (instanceData.size() && arrowMaterial.get_shader())
  {
    const Shader &arrowShader = arrowMaterial.get_shader();
    arrowShader.use();
    render_depth_case(instanceData, depthIgnore, true, wire_frame);
    render_depth_case(instanceData, depthNotIgnore, false, wire_frame);
  }
  depthIgnore.clear();
  depthNotIgnore.clear();
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
}
 */