
#include "engine/application.h"

#include "engine/logger.h"
#include "engine/renderer.h"

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

static platform_state *plat_state;

void application_initialize(platform_state *state) { plat_state = state; }

bool application_run() {
  while (!glfwWindowShouldClose(plat_state->window)) {
    glfwPollEvents();
    draw_frame();
  }
  OE_LOG(LOG_LEVEL_DEBUG, "Application terminating");
  return false;
}
