#ifndef __HAIR_H__
#define __HAIR_H__

// GLM includes
// This library provides primitive vector and matrix operations.
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

class Hair {
public:
  glm::vec3 top_center;
  glm::vec3 vertices[4];
  glm::vec3 color;

  float frequency;
  float phase;
};

#endif // __HAIR_H__