
#include "engine/application.h"

#include "engine/logger.h"
#include "engine/renderer.h"

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

static platform_state *plat_state;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_E && action == GLFW_PRESS)
    OE_LOG(LOG_LEVEL_DEBUG, "E KEY PRESSED");

  if (key == GLFW_KEY_Q && action == GLFW_PRESS) application_shutdown();
}
void application_initialize(platform_state *state) {
  plat_state = state;

  glfwSetKeyCallback(plat_state->window, key_callback);

  load_object();
  OE_LOG(LOG_LEVEL_INFO, "Application initialized!");
}

bool application_run() {
  while (!glfwWindowShouldClose(plat_state->window)) {
    glfwPollEvents();
    draw_frame();
  }
  OE_LOG(LOG_LEVEL_DEBUG, "Application terminating");
  return false;
}

void application_shutdown() {
  renderer_shutdown();
  glfwTerminate();
}
