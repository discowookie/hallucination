#include "hallucination.h"

void Hallucination::Init() {
  LoadModels();
  CreateOpenGLWindow();
  SetupLighting();
}

void Hallucination::LoadModels() {
  // Load OBJ model files for the human body, jacket, etc.
  std::cout << "Loading OBJ files..." << std::endl;
  human_body_obj_.Load("models/male1591.obj");
  eyes_obj_.Load("models/high-poly.obj");
  jacket_obj_.Load("models/tshirt_long.obj");
  jeans_obj_.Load("models/jeans01.obj");
  shoes_obj_.Load("models/shoes02.obj");
}

void Hallucination::CreateOpenGLWindow() {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  // TODO(wcraddock): do we need an error callback?
  // glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_SAMPLES, 4);

  window = glfwCreateWindow(window_width_, window_height_, "Hallucination",
                            NULL, NULL);
  if (!window) {
    printf("Oh noes!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  Controller::getInstance().RegisterCallbacks(window);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Call the cursor callback once, so it'll set up the view matrix.
  // TODO(wcraddock): this should probably be in an init() function somewhere.
  // cursor_position_callback(window, 0, 0);
}

void Hallucination::SetupLighting() {
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

void Hallucination::MainLoop() {}

Hallucination::~Hallucination() {}