#ifndef __HALLUCINATION_H__
#define __HALLUCINATION_H__

#include <vector>

// Disco Wookie includes
#include "hair.h"
#include "controller.h"
#include "audio.h"

// GLWFW includes
// This library simplifies OpenGL window creation, and mouse & keyboard input.
#include <GLFW/glfw3.h>

// GLM includes
// This library provides primitive vector and matrix operations.
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// OBJ reader includes
// This piece of code can read 3D models in the Wavefront .OBJ format.
#include "obj_reader.h"

// PortAudio includes
// This library provides a cross-platform way to read audio samples from
// the microphone, and from various file formats.
#include "portaudio.h"

// aubio includes
// This library provides a variety of audio signal annotations, such as
// onset detection, beat tracking, etc.
#include <aubio/aubio.h>
#include <aubio/fvec.h>
#include <aubio/onset/onset.h>

class Hallucination {
public:
  Hallucination()
      : window_width_(1024), window_height_(768), human_display_list_(0) {}

  ~Hallucination();

  void Init();
  void MainLoop();

private:
  void LoadModels();
  void CreateOpenGLWindow();
  void SetupLighting();
  void StartAudioProcessor();

  void Display();

  int window_width_;
  int window_height_;

  // OpenGL window object.
  GLFWwindow *window;

  // The display list for the whole human model (and clothes).
  GLuint human_display_list_;

  // Models for the human body and for the jacket.
  Model_OBJ human_body_obj_;
  Model_OBJ eyes_obj_;
  Model_OBJ jacket_obj_;
  Model_OBJ jeans_obj_;
  Model_OBJ shoes_obj_;

  // The Fur object contains a list of Hairs all over the jacket.
  Fur fur_;

  AudioProcessor   audio_processor_;
};

#endif // __HALLUCINIATION_H__