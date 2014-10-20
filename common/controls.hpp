#ifndef CONTROLS_HPP
#define CONTROLS_HPP

typedef enum {
  RANDOM_SINE_WAVES = 0,
  PHOTOGRAMMETRY = 1,
  BEAT_DETECTION = 2
} IlluminationMode;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void computeMatricesFromInputs(GLFWwindow* window);

glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
float getModelAngle();
IlluminationMode getIlluminationMode();

#endif