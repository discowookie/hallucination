#include <stdio.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "controls.hpp"

static float window_width = 1024.0f;
static float window_height = 768.0f;

const float comparison_epsilon = 0.001;

// Camera position, angle, and field-of-view.
static glm::vec3 camera_position = glm::vec3(0, 1.4f, 2.0f);
static float horizontalAngle = 3.14f;
static float verticalAngle = 0.0f;
static float initialFoV = 60.0f;

// Viewer's direction, plus her up and right vectors.
static glm::vec3 direction;
static glm::vec3 right;
static glm::vec3 up;

// These define the speed of movements done with the keyboard and mouse.
static float keyboard_speed = 0.1f;
static float mouse_speed = 0.00001f;

// The model can spin around under keyboard control.
static float model_angle = 0.0f;

// OpenGL model-view and projection matrices
static glm::mat4 ViewMatrix;
static glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() { return ViewMatrix; }
glm::mat4 getProjectionMatrix() { return ProjectionMatrix; }
float getModelAngle() { return model_angle; }

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  bool print = 0;

  if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    glfwSetWindowShouldClose(window, GL_TRUE);

  // Move forward
  if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (print)
      printf("Moving up...\n");
    camera_position += direction * keyboard_speed;
  }
  // Move backward
  if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (print)
      printf("Moving down...\n");
    camera_position -= direction * keyboard_speed;
  }

  // Spin model left
  if (key == GLFW_KEY_RIGHT &&
      (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    model_angle += 1.0f * keyboard_speed;
    // printf("Model angle is now %f\n", model_angle);
  }
  // Spin model right
  if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    model_angle -= 1.0f * keyboard_speed;
    // printf("Model angle is now %f\n", model_angle);
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

  // // Strafe right
  // if (key == GLFW_KEY_RIGHT &&
  //     (action == GLFW_PRESS || action == GLFW_REPEAT)) {
  //   if (print)
  //     printf("Moving right...\n");
  //   camera_position += right * keyboard_speed;
  // }
  // // Strafe left
  // if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action ==
  // GLFW_REPEAT)) {
  //   if (print)
  //     printf("Moving left...\n");
  //   camera_position -= right * keyboard_speed;
  // }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  bool print = 0;

  if (print)
    printf("Cursor position is %f %f\n", xpos, ypos);

  float horizontalAngleAdjustment =
      mouse_speed * float(window_width / 2.0 - xpos);
  float verticalAngleAdjustment =
      mouse_speed * float(window_height / 2.0 - ypos);
  if (print)
    printf("Adjusting angles by %f %f\n", horizontalAngleAdjustment,
           verticalAngleAdjustment);

  // Compute new orientation
  horizontalAngle += horizontalAngleAdjustment;
  verticalAngle += verticalAngleAdjustment;
  if (print)
    printf("Angles are %f %f\n", horizontalAngle, verticalAngle);

  if (abs(horizontalAngle - 0.0) > comparison_epsilon ||
      abs(verticalAngle - 0.0) > comparison_epsilon) {
    // printf("Mouse movement: horizontalAngle %f verticalAngle %f\n",
    // horizontalAngle, verticalAngle);
  }

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  direction =
      glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle),
                cos(verticalAngle) * cos(horizontalAngle));
  // glm::vec3 direction = glm::vec3(
  //                         glm::eulerAngleYX(horizontalAngle, verticalAngle)
  //                       * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
  if (print)
    printf("Camera direction: %f, %f, %f\n", direction.x, direction.y,
           direction.z);

  // Right vector
  right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0,
                    cos(horizontalAngle - 3.14f / 2.0f));

  if (print)
    printf("Right direction %f %f %f\n", right.x, right.y, right.z);

  // Up vector
  up = glm::cross(right, direction);
  if (print)
    printf("Up direction %f %f %f\n", up.x, up.y, up.z);

  if (print)
    printf("Position %f %f %f\n", camera_position.x, camera_position.y,
           camera_position.z);
}

void computeMatricesFromInputs(GLFWwindow *window) {
  bool print = 1;

  // glfwGetTime is called only once, the first time this function is called
  static double lastTime = glfwGetTime();

  // Compute time difference between current and last frame
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);

  float FoV = initialFoV;

  // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit
  // <-> 100 units
  // ProjectionMatrix = glm::ortho( -5.f, 5.f, -50.f, 50.f);
  ProjectionMatrix = glm::perspective(FoV, 4.0f / 3.0f, 0.1f, 500.0f);
  const char *s = glm::to_string(ProjectionMatrix).c_str();

  // Camera matrix
  ViewMatrix =
      glm::lookAt(camera_position,             // Camera is here
                  camera_position + direction, // and looks here : at the same
                                               // position, plus "direction"
                  up // Head is up (set to 0,-1,0 to look upside-down)
                  );

  // For the next frame, the "last time" will be "now"
  lastTime = currentTime;
}