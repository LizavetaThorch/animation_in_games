#pragma once
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vector_angle.hpp>
using namespace glm;

constexpr float PI = 3.1415926535897932384626433832795f;
constexpr float PIHALF = PI * 0.5f;
constexpr float PITWO = PI * 2.0f;
constexpr float DegToRad = PI / 180.f;
constexpr float RadToDeg = 1.f / DegToRad;
template<typename T>
vec2 to_vec2(const T& t)
{
  return vec2(t.x, t.y);
}
template<typename T>
vec3 to_vec3(const T& t)
{
  return vec3(t.x, t.y, t.z);
}
template<typename T>
vec4 to_vec4(const T& t)
{
  return vec4(t[0], t[1], t[2], t[3]);
}
template<typename T>
quat to_quat(const T& t)
{
  return quat(t.w, t.x, t.y, t.z);
}