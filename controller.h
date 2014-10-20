#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

// GLWFW includes
#include <GLFW/glfw3.h>

// GLM includes
// This library provides primitive vector and matrix operations.
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

class Controller {
public:
  typedef enum {
    RANDOM_SINE_WAVES = 0,
    PHOTOGRAMMETRY = 1,
    BEAT_DETECTION = 2
  } IlluminationMode;

  Controller()
      : camera_position_(glm::vec3(0, 1.2f, 1.5f)), horizontal_angle_(3.14f),
        vertical_angle_(0.0f), field_of_view_angle_(60.0f),
        keyboard_speed_(0.1f), mouse_speed_(0.00001f), model_angle_(0.0f),
        illumination_mode_(RANDOM_SINE_WAVES) {}

  static Controller &getInstance() {
    static Controller instance;
    return instance;
  }

  static void RegisterCallbacks(GLFWwindow *window) {
    glfwSetKeyCallback(window, KeyCallbackWrapper);
    glfwSetCursorPosCallback(window, CursorPositionCallbackWrapper);
  }

  void Init();

  void ComputeMatrices(GLFWwindow *window, glm::mat4 &projection_matrix,
                       glm::mat4 &view_matrix);

private:
  // Camera position, angle, and field-of-view.
  glm::vec3 camera_position_;
  float horizontal_angle_;
  float vertical_angle_;
  float field_of_view_angle_;

  // Viewer's direction, plus her up and right vectors.
  glm::vec3 direction_;
  glm::vec3 right_;
  glm::vec3 up_;

  // These define the speed of movements done with the keyboard and mouse.
  float keyboard_speed_;
  float mouse_speed_;

  // The model can spin around under keyboard control.
  float model_angle_;

  IlluminationMode illumination_mode_;

  void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
  void CursorPositionCallback(GLFWwindow *window, double xpos, double ypos);

  static void KeyCallbackWrapper(GLFWwindow *window, int key, int scancode,
                                 int action, int mods) {
    getInstance().KeyCallback(window, key, scancode, action, mods);
  }

  static void CursorPositionCallbackWrapper(GLFWwindow *window, double xpos,
                                            double ypos) {
    getInstance().CursorPositionCallback(window, xpos, ypos);
  }

  Controller(Controller const &);
  void operator=(Controller const &);
};

#endif // __CONTROLLER_H__