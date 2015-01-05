#include "hair.h"
#include "audio.h"
#include "debug.h"

#define TOTAL_FLOATS_IN_TRIANGLE 9

void Hair::Draw() const {
  // Set the emission intensity of the hair.
  // TODO(wcraddock): this might be slow. Does it matter?
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);

  // Draw the hair as a rectangle.
  glBegin(GL_QUADS);
  glVertex3f(vertices[0].x, vertices[0].y, vertices[0].z);
  glVertex3f(vertices[1].x, vertices[1].y, vertices[1].z);
  glVertex3f(vertices[2].x, vertices[2].y, vertices[2].z);
  glVertex3f(vertices[3].x, vertices[3].y, vertices[3].z);
  glEnd();
}

void Hair::SetGrey(float illumination) {
  color[0] = illumination;
  color[1] = illumination;
  color[2] = illumination;
}

void Fur::GenerateRandomHairs(Model_OBJ &obj, int num_hairs) {
  srand(time(NULL));

  while (hairs.size() < num_hairs) {
    // Pick a random face
    int total_faces = obj.TotalConnectedTriangles / 9;
    int face_number = rand() % total_faces;

    // Get the three vertices of the face
    float *vertex =
        &obj.Faces_Triangles[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 A(vertex[0], vertex[1], vertex[2]);
    glm::vec3 B(vertex[3], vertex[4], vertex[5]);
    glm::vec3 C(vertex[6], vertex[7], vertex[8]);

    // Choose a point somewhere on the face for the hair's location
    float r1 = ((double)rand() / (RAND_MAX));
    float r2 = ((double)rand() / (RAND_MAX));
    glm::vec3 top_center = A + r1 * (B - A) + r2 * (C - A);

    // If the point is too close to an existing hair, try again.
    float closest_hair = FindClosestHair(top_center);
    if (closest_hair < 0.0127f) {
      continue;
    }

    // Get the normal for that vertex.
    float *normal_f =
        &obj.normals[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 normal =
        glm::normalize(glm::vec3(normal_f[0], normal_f[1], normal_f[2]));

    normal = glm::normalize(glm::cross(B - A, C - A));

    // Cross the normal with the "straight down" direction to get a vector
    // that points left, along the width of the hair.
    glm::vec3 hair_left =
        glm::normalize(glm::cross(normal, glm::vec3(0.0f, -1.0f, 0.0f)));

    // Cross the left vector with the normal to obtain a vector along the
    // length of the hair.
    glm::vec3 hair_down = glm::normalize(glm::cross(hair_left, normal));

    const float hair_width = 0.0127f;  // 0.5 inch
    const float hair_height = 0.0762f; // 3 inches

    // Compute the four corners of the hair.
    glm::vec3 top_left = top_center - (hair_width / 2.0f) * hair_left;
    glm::vec3 bottom_left = top_left + hair_height * hair_down;
    glm::vec3 bottom_right = bottom_left + hair_width * hair_left;
    glm::vec3 top_right = bottom_right - hair_height * hair_down;

    // Create a Hair object and push it into the list of hairs.
    // Give each hair a random frequency and phase.
    Hair hair;
    hair.top_center = top_center;
    hair.vertices[0] = top_left;
    hair.vertices[1] = bottom_left;
    hair.vertices[2] = bottom_right;
    hair.vertices[3] = top_right;
    hair.frequency = 5.0f * ((double)rand() / (RAND_MAX));
    hair.phase = 3.14f * ((double)rand() / (RAND_MAX));

    hairs.push_back(hair);
  }
}

// Given a vertex, finds the distance to the nearest hair's top-center
// vertex. Used to scatter the hairs evenly across the jacket.
float Fur::FindClosestHair(glm::vec3 &vertex) {
  float min_distance = FLT_MAX;

  for (unsigned int i = 0; i < hairs.size(); ++i) {
    float distance = glm::length(vertex - hairs[i].top_center);
    if (distance < min_distance) {
      min_distance = distance;
    }
  }

  return min_distance;
}

// This method is called by the OpenGL main loop to draw all of the hairs.
// It checks the audio processor for events, determines the brightness of
// each hair, and makes the OpenGL calls to draw the hair with its new
// brightness.
void Fur::DrawHairs(Visualizer* visualizer) {
  const double time = glfwGetTime();
  visualizer->Draw(time);
}
