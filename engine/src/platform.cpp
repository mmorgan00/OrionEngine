

#define GLFW_INCLUDE_VULKAN

#include "engine/platform.h"

#include <stdlib.h>

#include <vector>

#include "engine/logger.h"

static platform_state *plat_state;

platform_state *platform_initialize() {
  plat_state = (platform_state *)malloc(sizeof(platform_state));

  if (!glfwInit()) {
    OE_LOG(LOG_LEVEL_FATAL, "GLFW failed to init.");
    // TODO: Kill app from here
    return plat_state;
  };

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWmonitor *primary = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  plat_state->window =
      glfwCreateWindow(mode->width, mode->height, "Orion", primary, NULL);

  if (!plat_state->window) {
    glfwTerminate();
    OE_LOG(LOG_LEVEL_FATAL, "Failed to initialize platform. Unrecoverable");
    // TODO: kill app from here somehow
    return plat_state;
  }

  OE_LOG(LOG_LEVEL_INFO, "Platform Initialized!");

  // This will be passed to other systems, render, input, etc
  return plat_state;
}

/**
 * @brief creates a window according to the platform requirements.
 * @detail Creates the GLFW window and returns the shared reference that the
 * platform state uses
 */
void platform_shutdown() {
  glfwDestroyWindow(plat_state->window);

  glfwTerminate();
}

/**
 * @brief Reads a file and returns the contents as a buffer
 * @returns std::vector of chars
 */
std::vector<char> platform_read_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}
