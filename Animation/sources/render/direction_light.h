#pragma once
#include "3dmath.h"


struct DirectionLight
{
  vec3 lightDirection;
  vec3 lightColor;
  vec3 ambient;
};