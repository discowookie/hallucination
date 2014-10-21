#include "hair.h"
#include "controller.h"
#include "audio.h"

#define TOTAL_FLOATS_IN_TRIANGLE 9

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

    glm::vec3 top_left = top_center - (hair_width / 2.0f) * hair_left;
    glm::vec3 bottom_left = top_left + hair_height * hair_down;
    glm::vec3 bottom_right = bottom_left + hair_width * hair_left;
    glm::vec3 top_right = bottom_right - hair_height * hair_down;

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

void Fur::DrawHairs(AudioProcessor& audio) {
  static int lit_hair = 0;
  static double last_hair_change_time = 0;

  static double prev_time = 0;
  double time = glfwGetTime();

  Controller::IlluminationMode illuminationMode =
    Controller::getInstance().GetIlluminationMode();
  
  // Get the result of the audio tempo detector.
  float last_beat_s, tempo_bpm, confidence;
  bool is_beat = audio.IsBeat(last_beat_s, tempo_bpm, confidence);

  if (is_beat) {
    static int num_beats = 0;
    printf("beat %d: time %.3f s, tempo %.2f bpm, confidence %.2f\n",
           num_beats++, last_beat_s, tempo_bpm, confidence);
  }

  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair &hair = hairs[i];

    float illumination = 0.0f;

    if (illuminationMode == Controller::PHOTOGRAMMETRY) {
      // In this mode, each hair is lit for 1/10th of a second. The hairs
      // are cycled through in random order.
      if (time - last_hair_change_time > 0.1f) {
        lit_hair = (lit_hair + 1) % hairs.size();
        last_hair_change_time = time;
      }

      illumination = (i == lit_hair) ? 1.0f : 0.0f;
    } else if (illuminationMode == Controller::RANDOM_SINE_WAVES) {
      illumination = sin(hair.frequency * time + hair.phase);
    } else if (illuminationMode == Controller::BEAT_DETECTION) {
      if (is_beat) {
        // Pick random hairs to light up to max brightness.
        float r = ((double)rand() / (RAND_MAX));
        if (r > 0.7f)
          hairs[i].illumination = 1.0f;
      } else {
        // If there is no beat, make all the hairs decay in brightness.
        hairs[i].illumination *= (31.0f / 32.0f);
      }

      illumination = 2.0f * hairs[i].illumination - 1.0f;
    }

    // Set the emission intensity of the hair.
    // TODO(wcraddock): this might be slow. Does it matter?
    GLfloat color[3] = { illumination, illumination, illumination };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);

    // Draw the hair as a rectangle.
    glBegin(GL_QUADS);
    glVertex3f(hair.vertices[0].x, hair.vertices[0].y, hair.vertices[0].z);
    glVertex3f(hair.vertices[1].x, hair.vertices[1].y, hair.vertices[1].z);
    glVertex3f(hair.vertices[2].x, hair.vertices[2].y, hair.vertices[2].z);
    glVertex3f(hair.vertices[3].x, hair.vertices[3].y, hair.vertices[3].z);
    glEnd();
  }

  prev_time = time;
}