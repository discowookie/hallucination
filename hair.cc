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

Fur::Fur(AudioProcessor* audio)
  : audio_(audio) {}

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
// TODO(wcraddock): this is too much to do in one function.
void Fur::DrawHairs(Controller::IlluminationMode mode) {
  // State for the photogrammetry.
  static int lit_hair = 0;
  static double last_hair_change_time = 0;

  // Get the current time from OpenGL.
  static double prev_time = 0;
  double time = glfwGetTime();

  //
  // Part 1. Determine confidence that some audio event has happened.
  // The onset detector is checked first, and it assigns a confidence
  // value. The beat detector is checked second, and its confidence
  // overrides that from the onset detector.
  // 
  // The idea is that the Disco Wookie should fall back on onset
  // detection when the beat is not know with any confidence.
  // 
  // TODO(wcraddock): These operations should be broken up into a simple
  // API so that everyone can easily try out visualization code.

  float confidence = 0.0f;

  // The audio processor tells us when an onset event has occurred since the 
  // last time through this OpenGL display loop.
  float last_onset_s;
  bool is_onset = audio_->IsOnset(last_onset_s);
  if (is_onset) {
    static int num_onsets = 0;
    if (DEBUG_MODE) {
      printf("onset %d: time %.3f s\n", num_onsets++, last_onset_s);
    }

    // The aubio library does not provide confidence values for onsets.
    // TODO(wcraddock): what the hell is the right idea here?
    confidence = 0.5f;
  }

  // The audio processor tells us when a beat event has occurred since the 
  // last time through this OpenGL display loop.
  float beat_confidence;
  float last_beat_s, tempo_bpm;
  bool is_beat = audio_->IsBeat(last_beat_s, tempo_bpm, beat_confidence);
  if (is_beat) {
    // If the beat_confidence is very low, don't count it as a beat at all.
    // Otherwise, make it a strong visual event by giving it high confidence.
    if (beat_confidence >= 0.2f) {
      confidence = 1.0f;
      static int num_beats = 0;
      if (DEBUG_MODE) {
        printf("beat %d: time %.3f s, tempo %.2f bpm, confidence %.2f\n",
              num_beats++, last_beat_s, tempo_bpm, confidence);
      }
    }
  }

  //
  // Part 2. CHange the illumination of each of the hairs in turn. Use 
  // the illumination mode, the confidence of the audio detectors,
  // and possibly black magic to determine each hair's new brightness.
  // 
  // It is a game of creating a recurrence relation:
  //     brightness(now) = f( brightness(last time), confidence, black_magic )
  // 
  // TODO(wcraddock): Abstract this out so everyone can easily write
  // visualization code.

  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair &hair = hairs[i];

    float illumination = 0.0f;

    if (mode == Controller::PHOTOGRAMMETRY) {
      // In this mode, each hair is lit for 1/10th of a second. The hairs
      // are cycled through in random order.
      if (time - last_hair_change_time > 0.1f) {
        lit_hair = (lit_hair + 1) % hairs.size();
        last_hair_change_time = time;
      }

      illumination = (i == lit_hair) ? 1.0f : 0.0f;
    } else if (mode == Controller::RANDOM_SINE_WAVES) {
      illumination = sin(hair.frequency * time + hair.phase);
    } else if (mode == Controller::BEAT_DETECTION) {
      if (is_onset || is_beat) {
        // Pick random hairs to light up to max brightness. Add the confidence
        // to it, to make it brighter.
        float r = ((double)rand() / (RAND_MAX));
        if (r > 0.8f)
          hairs[i].illumination =
            std::min(hairs[i].illumination + confidence, 1.0f);
      } else {
        // If there is no beat or onset, make all the hairs decay in brightness.
        // Decays to 0.0f. Make this ratio closer to 1 to make the decay slower.
        hairs[i].illumination *= (63.0f / 64.0f);
      }

      illumination = hairs[i].illumination;
      illumination = 2.0f * illumination - 1.0f;
    }

    hair.SetGrey(illumination);

    // Draw the hair as a rectangle.
    hair.Draw();
  }

  prev_time = time;
}
