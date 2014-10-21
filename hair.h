#ifndef __HAIR_H__
#define __HAIR_H__

#include <vector>

// Disco Wookie includes
#include "obj_reader.h"

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

// The Fur class represents a collection of Hairs. It includes methods to
// create new Hairs, etc.
class Fur {
public:
  Fur() {}

  // Given some model object, create a bunch of hairs all over it.
  void GenerateRandomHairs(Model_OBJ &obj, int num_hairs);

  // Return the distance to the hair closest to the given point in space.
  float FindClosestHair(glm::vec3 &vertex);

  // Draw all the of the hairs with OpenGL.
  void DrawHairs();

  std::vector<Hair>     hairs;
};

#endif // __HAIR_H__