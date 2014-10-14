#include <stdio.h>

// Include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow *window; // The "extern" keyword here is to access the
                           // variable "window" declared in tutorialXXX.cpp.
                           // This is a hack to keep the tutorials simple.
                           // Please avoid this.

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
using namespace glm;

#include "controls.hpp"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() { return ViewMatrix; }
glm::mat4 getProjectionMatrix() { return ProjectionMatrix; }

const float comparison_epsilon = 0.001;

glm::vec3 position = glm::vec3(0, 0.5f, 5.0f);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 90.0f;

float speed = 0.1f;
float mouseSpeed = 0.00001f;

glm::vec3 direction;
glm::vec3 right;
glm::vec3 up;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    glfwSetWindowShouldClose(window, GL_TRUE);

  // Move forward
  if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    printf("Moving up...\n");
    position += direction * speed;
  }
  // Move backward
  if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    printf("Moving down...\n");
    position -= direction * speed;
  }
  // Strafe right
  if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    printf("Moving right...\n");
    position += right * speed;
  }
  // Strafe left
  if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    printf("Moving left...\n");
    position -= right * speed;
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  bool print = 1;

  // // Get mouse position
  // double xpos, ypos;
  // glfwGetCursorPos(window, &xpos, &ypos);

  // // Reset mouse position for next frame
  // // TODO(wcraddock): get real window size here.
  // glfwSetCursorPos(window, 640.0 / 2.0, 480.0 / 2.0);

  // if (xpos == 0.0 && ypos == 0.0) {
  //   return;
  // }

  // // if (xpos != 320.0 || ypos != 240.0) {
  // //   print = 1;
  // //   xpos = 320.0;
  // //   ypos = 240.0;
  // // }

  if (print)
    printf("Cursor position is %f %f\n", xpos, ypos);


  float horizontalAngleAdjustment = mouseSpeed * float(640.0 / 2.0 - xpos);
  float verticalAngleAdjustment = mouseSpeed * float(480.0 / 2.0 - ypos);
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
  direction = glm::vec3(cos(verticalAngle) * sin(horizontalAngle),
                      sin(verticalAngle),
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
    printf("Position %f %f %f\n", position.x, position.y, position.z);
}

void computeMatricesFromInputs(GLFWwindow *window) {
  bool print = 1;

  // glfwGetTime is called only once, the first time this function is called
  static double lastTime = glfwGetTime();

  // Compute time difference between current and last frame
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);


  float FoV = initialFoV; // - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires
                          // setting up a callback for this. It's a bit too
                          // complicated for this beginner's tutorial, so it's
                          // disabled instead.
  // printf("FoV %f\n", FoV);

  // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit
  // <-> 100 units
  // ProjectionMatrix = glm::ortho( -5.f, 5.f, -50.f, 50.f);
  ProjectionMatrix = glm::perspective(FoV, 4.0f / 3.0f, 0.1f, 500.0f);
  const char *s = glm::to_string(ProjectionMatrix).c_str();
  // printf("Projection matrix: %s\n", s);

  // Camera matrix
  ViewMatrix = glm::lookAt(position,             // Camera is here
                           position + direction, // and looks here : at the same
                                                 // position, plus "direction"
                           up // Head is up (set to 0,-1,0 to look upside-down)
                           );

  // For the next frame, the "last time" will be "now"
  lastTime = currentTime;
}