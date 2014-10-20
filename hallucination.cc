#include "hallucination.h"

void Hallucination::Init() {
  LoadModels();
  CreateOpenGLWindow();
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

  controller_.RegisterCallbacks(window);

  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Call the cursor callback once, so it'll set up the view matrix.
  // TODO(wcraddock): this should probably be in an init() function somewhere.
  // cursor_position_callback(window, 0, 0);
}

void Hallucination::Run() {}

Hallucination::~Hallucination() {}