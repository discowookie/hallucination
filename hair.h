#ifndef __HAIR_H__
#define __HAIR_H__

#include <vector>

// Disco Wookie includes
#include "audio.h"
#include "controller.h"
#include "obj_reader.h"

// GLM includes
// This library provides primitive vector and matrix operations.
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

using std::vector;

class Hair {
 public:
  // Render the hair in OpenGL.
  void Draw() const;

  // Set a grey scale color.
  void SetGrey(float illumination);

  // Modified by visualizers to control lighting.
  GLfloat color[3];

  // Modified only by Fur only once after instantiation.
  vec3 top_center;
  vec3 vertices[4];
};

// Fur is a collection of Hairs
class Fur {
 public:
  // Given some model object, create a bunch of hairs all over it.
  void GenerateRandomHairs(Model_OBJ &obj, int num_hairs);

  vector<Hair> hairs;
};

#endif // __HAIR_H__
