#include "controller.h"

#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Controller::KeyCallback(GLFWwindow *window, int key, int scancode,
                             int action, int mods) {
  bool print = 0;

  if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    glfwSetWindowShouldClose(window, GL_TRUE);

  // Move forward
  if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (print)
      printf("Moving up...\n");
    camera_position_ += direction_ * keyboard_speed_;
  }
  // Move backward
  if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (print)
      printf("Moving down...\n");
    camera_position_ -= direction_ * keyboard_speed_;
  }

  // Spin model left
  if (key == GLFW_KEY_RIGHT &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    model_angle_ += 1.0f * keyboard_speed_;
    // printf("Model angle is now %f\n", model_angle_);
  }
  // Spin model right
  if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    model_angle_ -= 1.0f * keyboard_speed_;
    // printf("Model angle is now %f\n", model_angle_);
  }

  // Turn lights on and off
  if (key == GLFW_KEY_L && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    static bool lights_on = true;
    if (lights_on) {
      glDisable(GL_LIGHT0);
    } else {
      glEnable(GL_LIGHT0);
    }
    lights_on = !lights_on;
  }

  // Change illumination modes.
  if (key == GLFW_KEY_1 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    illumination_mode_ = RANDOM_SINE_WAVES;
  }

  if (key == GLFW_KEY_2 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    illumination_mode_ = BEAT_DETECTION;
  }

  if (key == GLFW_KEY_3 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    illumination_mode_ = PHOTOGRAMMETRY;
  }

  // // Strafe right
  // if (key == GLFW_KEY_RIGHT &&
  //     (action == GLFW_PRESS || action == GLFW_REPEAT)) {
  //   if (print)
  //     printf("Moving right...\n");
  //   camera_position_ += right * keyboard_speed_;
  // }
  // // Strafe left
  // if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action ==
  // GLFW_REPEAT)) {
  //   if (print)
  //     printf("Moving left...\n");
  //   camera_position_ -= right * keyboard_speed_;
  // }
}

void Controller::CursorPositionCallback(GLFWwindow *window, double xpos,
                                        double ypos) {
  bool print = 0;

  const float comparison_epsilon = 0.0001f;

  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);

  if (print)
    printf("Cursor position is %f %f\n", xpos, ypos);

  float horizontalAngleAdjustment =
      mouse_speed_ * float(window_width / 2.0 - xpos);
  float verticalAngleAdjustment =
      mouse_speed_ * float(window_height / 2.0 - ypos);
  if (print)
    printf("Adjusting angles by %f %f\n", horizontalAngleAdjustment,
           verticalAngleAdjustment);

  // Compute new orientation
  horizontal_angle_ += horizontalAngleAdjustment;
  vertical_angle_ += verticalAngleAdjustment;
  if (print)
    printf("Angles are %f %f\n", horizontal_angle_, vertical_angle_);

  if (abs(horizontal_angle_ - 0.0) > comparison_epsilon ||
      abs(vertical_angle_ - 0.0) > comparison_epsilon) {
    // printf("Mouse movement: horizontal_angle_ %f vertical_angle_ %f\n",
    // horizontal_angle_, vertical_angle_);
  }

  // Direction_ : Spherical coordinates to Cartesian coordinates conversion
  direction_ = glm::vec3(cos(vertical_angle_) * sin(horizontal_angle_),
                         sin(vertical_angle_),
                         cos(vertical_angle_) * cos(horizontal_angle_));
  // glm::vec3 direction_ = glm::vec3(
  //                         glm::eulerAngleYX(horizontal_angle_,
  // vertical_angle_)
  //                       * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
  if (print)
    printf("Camera direction: %f, %f, %f\n", direction_.x, direction_.y,
           direction_.z);

  // Right vector
  right_ = glm::vec3(sin(horizontal_angle_ - 3.14f / 2.0f), 0,
                     cos(horizontal_angle_ - 3.14f / 2.0f));

  if (print)
    printf("Right_ direction %f %f %f\n", right_.x, right_.y, right_.z);

  // Up vector
  up_ = glm::cross(right_, direction_);
  if (print)
    printf("Up direction %f %f %f\n", up_.x, up_.y, up_.z);

  if (print)
    printf("Position %f %f %f\n", camera_position_.x, camera_position_.y,
           camera_position_.z);
}

void Controller::ComputeMatrices(GLFWwindow *window,
                                 glm::mat4 &projection_matrix,
                                 glm::mat4 &model_matrix,
                                 glm::mat4 &view_matrix) {
  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);

  float aspect_ratio =
      static_cast<float>(window_width) / static_cast<float>(window_height);

  projection_matrix =
      glm::perspective(field_of_view_angle_, aspect_ratio, 0.1f, 500.0f);

  // Camera matrix
  view_matrix =
      glm::lookAt(camera_position_,              // Camera is here
                  camera_position_ + direction_, // and looks here : at the same
                                                 // position, plus "direction"
                  up_ // Head is up (set to 0,-1,0 to look upside-down)
                  );

  // Model matrix (we only handle rotation here).
  model_matrix =
      glm::rotate(glm::mat4(1.0), model_angle_, glm::vec3(0.0f, 1.0f, 0.0f));
}