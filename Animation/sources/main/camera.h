#pragma once
#include "3dmath.h"


struct ArcballCamera
{
  float curZoom;
  float maxdistance;
  float targetZoom;
  float distance;
  float lerpStrength;
  float mouseSensitivity;
  float wheelSensitivity;
  vec2 targetRotation;
  vec3 targetPosition;
  bool rotationEnable;
  vec2 curRotation;
};

glm::mat4 calculate_transform(const ArcballCamera &camera);

void arcball_camera_update(
  ArcballCamera &arcballCamera,
  glm::mat4 &transform,
  float dt);

#include <input.h>
void arccam_mouse_wheel_handler(
  const SDL_MouseWheelEvent &e,
  ArcballCamera &arcballCamera);

void arccam_mouse_click_handler(
  const SDL_MouseButtonEvent &e,
  ArcballCamera &arcballCamera);

void arccam_mouse_move_handler(
  const SDL_MouseMotionEvent &e,
  ArcballCamera &arcballCamera);
