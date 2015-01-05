#include "hallucination.h"

Hallucination::Hallucination()
  : window_width_(1024),
    window_height_(768),
    human_display_list_(0),
    photogrammetry_(&fur_),
    random_waves_(&fur_),
    beats_(&fur_, &audio_processor_) {}

void Hallucination::Init() {
  LoadModels();
  CreateOpenGLWindow();
  SetupLighting();
  StartAudioProcessor();
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

void Hallucination::Display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Create a display list for the human body and jacket objects,
  // which never change.
  if (human_display_list_ == 0) {
    printf("Creating jacket and body display list...\n");
    human_display_list_ = glGenLists(1);
    glNewList(human_display_list_, GL_COMPILE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the emission of these polygons to zero; they'll be lit by diffuse
    // and ambient light.
    GLfloat black[3] = { 0.0f, 0.0f, 0.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Draw the body in a skin-tone color.
    glColor3f(1.0f, 0.86f, 0.69f);
    human_body_obj_.Draw();

    // // Draw the eyes in a ??? color.
    // glColor3f(1.0f, 1.0f, 1.0f);
    // eyes_obj.Draw();

    // Draw the jeans in a blue color.
    glColor3f(0.14f, 0.25f, 0.32f);
    jeans_obj_.Draw();

    // Draw the jacket in a dark charcoal color.
    glColor3f(0.25f, 0.25f, 0.25f);
    jacket_obj_.Draw();

    // Draw the jacket in a dark charcoal color.
    glColor3f(0.25f, 0.25f, 0.25f);
    shoes_obj_.Draw();

    glEndList();

    // Create the randomized hairs
    fur_.GenerateRandomHairs(jacket_obj_, 2400);
    photogrammetry_.Reposition();
    random_waves_.Reposition();
    beats_.Reposition();
  }

  // Draw the human (and clothing), then the hairs.
  glCallList(human_display_list_);
  const Controller& controller(Controller::getInstance());
  Controller::IlluminationMode mode = controller.GetIlluminationMode();
  const double time = glfwGetTime();
  if (mode == Controller::PHOTOGRAMMETRY) {
    photogrammetry_.Draw(time);
  } else if (mode == Controller::RANDOM_SINE_WAVES) {
    random_waves_.Draw(time);
  } else if (mode == Controller::BEAT_DETECTION) {
    beats_.Draw(time);
  } else {
    assert(false);
  }
}

void Hallucination::StartAudioProcessor() {
  audio_processor_.Init();
}

void Hallucination::MainLoop() {
  printf("Entering main loop...\n");
  while (!glfwWindowShouldClose(window)) {
    glm::mat4 projection_matrix, view_matrix, model_matrix;
    Controller::getInstance().ComputeMatrices(window, projection_matrix,
                                              model_matrix, view_matrix);

    // Set the model-view and projection matrices.
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&projection_matrix[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glm::mat4 MV = view_matrix * model_matrix;
    glLoadMatrixf(&MV[0][0]);

    Display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

Hallucination::~Hallucination() {
  // Tear down GLFW
  glfwDestroyWindow(window);
  glfwTerminate();
}
