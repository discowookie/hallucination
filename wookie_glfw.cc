// GLWFW includes
#include <GLFW/glfw3.h>

// GLM includes
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// PortAudio includes
#include "portaudio.h"

// Aubio includes
#include <aubio/aubio.h>
#include <aubio/fvec.h>
#include <aubio/onset/onset.h>

#include <stdlib.h>
#include <stdio.h>

#include "common/controls.hpp"
#include "common/obj_reader.h"

#include <vector>

// TODO(wcraddock): all of the OpenGL window params should really be up here.
// TODO(wcraddock): put all this static stuff into a class.
static int window_width = 1024.0f;
static int window_height = 768.0f;

// The display list for the whole human model (and clothes).
static GLuint human_display_list = 0;

// Models for the human body and for the jacket.
static Model_OBJ human_body_obj;
static Model_OBJ eyes_obj;
static Model_OBJ jacket_obj;
static Model_OBJ jeans_obj;
static Model_OBJ shoes_obj;

// Aubio onset detector and state.
static fvec_t *onset_out;
static aubio_onset_t *onset_obj;

// Aubio beat detector and state.
static fvec_t *tempo_out;
static aubio_tempo_t *tempo_obj;

static void error_callback(int error, const char *description) {
  fputs(description, stderr);
}

#define TOTAL_FLOATS_IN_TRIANGLE 9

// Keep track of the all hair locations that have already been selected.
class Hair {
public:
  glm::vec3 top_center;
  glm::vec3 vertices[4];
  glm::vec3 color;

  float frequency;
  float phase;
};

static std::vector<Hair> hairs;

float find_closest_hair(glm::vec3 &vertex) {
  float min_distance = FLT_MAX;

  for (unsigned int i = 0; i < hairs.size(); ++i) {
    float distance = glm::length(vertex - hairs[i].top_center);
    if (distance < min_distance) {
      min_distance = distance;
    }
  }

  return min_distance;
}

void generate_random_hairs(int num_hairs) {
  srand(time(NULL));

  while (hairs.size() < num_hairs) {
    // Pick a random face
    int total_faces = jacket_obj.TotalConnectedTriangles / 9;
    int face_number = rand() % total_faces;

    // Get the three vertices of the face
    float *vertex =
        &jacket_obj.Faces_Triangles[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 A(vertex[0], vertex[1], vertex[2]);
    glm::vec3 B(vertex[3], vertex[4], vertex[5]);
    glm::vec3 C(vertex[6], vertex[7], vertex[8]);

    // Choose a point somewhere on the face for the hair's location
    float r1 = ((double)rand() / (RAND_MAX));
    float r2 = ((double)rand() / (RAND_MAX));
    glm::vec3 top_center = A + r1 * (B - A) + r2 * (C - A);

    // If the point is too close to an existing hair, try again.
    float closest_hair = find_closest_hair(top_center);
    if (closest_hair < 0.0127f) {
      continue;
    }

    // Get the normal for that vertex.
    float *normal_f =
        &jacket_obj.normals[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 normal =
        glm::normalize(glm::vec3(normal_f[0], normal_f[1], normal_f[2]));
    // printf("normal from file: %f %f %f\n", normal.x, normal.y, normal.z);

    normal = glm::normalize(glm::cross(B - A, C - A));
    // printf("computed normal: %f %f %f\n", normal.x, normal.y, normal.z);

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

    // printf("top_center %f %f %f\n", top_center.x, top_center.y,
    // top_center.z);
    // printf("normal %f %f %f\n", normal.x, normal.y, normal.z);
    // printf("hair_left %f %f %f\n", hair_left.x, hair_left.y, hair_left.z);
    // printf("hair_down %f %f %f\n", hair_down.x, hair_down.y, hair_down.z);
    // printf("top_left %f %f %f\n", top_left.x, top_left.y, top_left.z);
    // printf("bottom_left %f %f %f\n", bottom_left.x, bottom_left.y,
    //        bottom_left.z);
    // printf("bottom_right %f %f %f\n", bottom_right.x, bottom_right.y,
    //        bottom_right.z);
    // printf("top_right %f %f %f\n", top_right.x, top_right.y, top_right.z);

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

void draw_hairs() {
  static int lit_hair = 0;
  static double last_hair_change_time = 0;

  static double prev_time = 0;
  double time = glfwGetTime();

  IlluminationMode illuminationMode = getIlluminationMode();

  static float global_illumination = 0.0f;
  if (getIlluminationMode() == BEAT_DETECTION) {
    // If the visualization mode is BEAT_DETECTION, get the result of the
    // audio onset and tempo detectors.
    smpl_t is_onset = fvec_get_sample(onset_out, 0);
    smpl_t is_beat = fvec_get_sample(tempo_out, 0);

    // Whenever a beat occurs, make all the hairs flash and slowly decay.
    global_illumination =
        is_beat ? 1.0f : global_illumination * (15.0f / 16.0f);

    if (is_beat) {
      static int num_beats = 0;
      printf("beat %d!\n", num_beats++);
    }
  }

  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair &hair = hairs[i];

    float illumination = 0.0f;

    if (illuminationMode == PHOTOGRAMMETRY) {
      // In this mode, each hair is lit for 1/10th of a second. The hairs
      // are cycled through in random order.
      if (time - last_hair_change_time > 0.1f) {
        lit_hair = (lit_hair + 1) % hairs.size();
        last_hair_change_time = time;
      }

      illumination = (i == lit_hair) ? 1.0f : 0.0f;
    } else if (illuminationMode == RANDOM_SINE_WAVES) {
      illumination = sin(hair.frequency * time + hair.phase);
    } else if (illuminationMode == BEAT_DETECTION) {
      illumination = global_illumination;
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

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Create a display list for the human body and jacket objects,
  // which never change.
  if (human_display_list == 0) {
    printf("Creating jacket and body display list...\n");
    human_display_list = glGenLists(1);
    glNewList(human_display_list, GL_COMPILE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the emission of these polygons to zero; they'll be lit by diffuse
    // and ambient light.
    GLfloat black[3] = { 0.0f, 0.0f, 0.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Draw the body in a skin-tone color.
    glColor3f(1.0f, 0.86f, 0.69f);
    human_body_obj.Draw();

    // // Draw the eyes in a ??? color.
    // glColor3f(1.0f, 1.0f, 1.0f);
    // eyes_obj.Draw();

    // Draw the jeans in a blue color.
    glColor3f(0.14f, 0.25f, 0.32f);
    jeans_obj.Draw();

    // Draw the jacket in a dark charcoal color.
    glColor3f(0.25f, 0.25f, 0.25f);
    jacket_obj.Draw();

    // Draw the jacket in a dark charcoal color.
    glColor3f(0.25f, 0.25f, 0.25f);
    shoes_obj.Draw();

    glEndList();

    // Create the randomized hairs
    generate_random_hairs(2400);
  }

  // Draw the human (and clothing), then the hairs.
  glCallList(human_display_list);
  draw_hairs();
}

void initialize_OpenGL() {
  // These constants define the OpenGL window.
  GLint width = 1024;
  GLint height = 768;
  char *title = "Disco Wookie";
  GLfloat field_of_view_angle = 45;
  GLfloat z_near = 1.0f;
  GLfloat z_far = 500.0f;

  // Set up z-buffering
  glShadeModel(GL_SMOOTH);
  glClearColor(0.0f, 0.1f, 0.0f, 0.5f);
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  // Set up lighting
  GLfloat amb_light[] = { 0.1, 0.1, 0.1, 1.0 };
  GLfloat diffuse[] = { 0.6, 0.6, 0.6, 1 };
  GLfloat specular[] = { 0.7, 0.7, 0.3, 1 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb_light);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glShadeModel(GL_SMOOTH);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

static float *overlap_buffer;

/* This routine will be called by the PortAudio engine when audio is needed.
   It may called at interrupt level on some machines so don't do anything
   that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData) {
  float *in = (float *)inputBuffer;

  // Copy the last half of the buffer to the front half.
  int winsize = 1024;
  memcpy(&overlap_buffer[0], &overlap_buffer[winsize / 2],
         sizeof(float) * (winsize / 2));

  // Copy the new data in to the last half of the overlap buffer.
  memcpy(&overlap_buffer[winsize / 2], in, sizeof(float) * (winsize / 2));

  // Run the aubio onset and beat detectors on the overlap buffer.
  fvec_t in_vec = { winsize, overlap_buffer };
  aubio_onset_do(onset_obj, &in_vec, onset_out);
  aubio_tempo_do(tempo_obj, &in_vec, tempo_out);
  // is_beat = fvec_get_sample (tempo_out, 0);

  return 0;
}

int initialize_audio() {
  // Initialize PortAudip
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    return err;
  }

  // TODO(wcraddock): put these parameters into the class constructor.
  uint_t win_size = 1024;
  uint_t step_size = win_size / 2;
  uint_t sample_rate = 44100;

  // Open an audio I/O stream for one input (microphone).
  PaStream *stream;
  err = Pa_OpenDefaultStream(
      &stream, 1,           /* mono input */
      0,                    /* no output channels */
      paFloat32,            /* 32 bit floating point output */
      sample_rate, win_size, /* frames per buffer, i.e. the number
                               of sample frames that PortAudio will
                               request from the callback. Many apps
                               may want to use
                               paFramesPerBufferUnspecified, which
                               tells PortAudio to pick the best,
                               possibly changing, buffer size.*/
      patestCallback, /* this is your callback function */
      NULL);         /* This is a pointer that will be passed to the callback */
  if (err != paNoError)
    return err;

  // Start the input audio stream
  err = Pa_StartStream(stream);

  // Create the aubio onset detector
  onset_out = new_fvec(1);
  onset_obj = new_aubio_onset("default", win_size, step_size, sample_rate);
  aubio_onset_set_threshold(onset_obj, 0.0f);
  aubio_onset_set_silence(onset_obj, -90.0f);

  // Create the aubio beat detector.
  tempo_out = new_fvec(2);
  overlap_buffer = new float[win_size];
  tempo_obj = new_aubio_tempo("default", win_size, step_size, sample_rate);
  aubio_tempo_set_threshold(tempo_obj, -10.0f);
  // aubio_tempo_set_silence (tempo_obj, -90.0f);

  return err;
}

int main(void) {
  printf("Loading OBJ files...\n");
  human_body_obj.Load("models/male1591.obj");
  eyes_obj.Load("models/high-poly.obj");
  jacket_obj.Load("models/tshirt_long.obj");
  jeans_obj.Load("models/jeans01.obj");
  shoes_obj.Load("models/shoes02.obj");

  printf("Creating OpenGL window...\n");
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_SAMPLES, 4);

  GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                        "Simple example", NULL, NULL);
  if (!window) {
    printf("Oh noes!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  glfwSetCursorPosCallback(window, cursor_position_callback);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Call the cursor callback once, so it'll set up the view matrix.
  // TODO(wcraddock): this should probably be in an init() function somewhere.
  cursor_position_callback(window, 0, 0);

  printf("Initializing OpenGL()...\n");
  initialize_OpenGL();

  // Initialize PortAudio
  printf("Initializing PortAudio...\n");
  initialize_audio();

  printf("Entering main loop...\n");
  while (!glfwWindowShouldClose(window)) {
    // Compute the MVP matrix from keyboard and mouse input
    computeMatricesFromInputs(window);
    glm::mat4 ProjectionMatrix = getProjectionMatrix();
    glm::mat4 ViewMatrix = getViewMatrix();
    glm::mat4 ModelMatrix = glm::rotate(glm::mat4(1.0), getModelAngle(),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 MV = ViewMatrix * ModelMatrix;

    // Set the MVP matrix
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&ProjectionMatrix[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MV[0][0]);

    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Tear down GLFW
  glfwDestroyWindow(window);
  glfwTerminate();

  // Tear down PortAudio
  PaError err = Pa_Terminate();
  if (err != paNoError)
    printf("PortAudio error: %s\n", Pa_GetErrorText(err));

  exit(EXIT_SUCCESS);
}
