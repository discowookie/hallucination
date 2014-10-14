#ifndef CONTROLS_HPP
#define CONTROLS_HPP

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void computeMatricesFromInputs(GLFWwindow* window);

glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();

#endif