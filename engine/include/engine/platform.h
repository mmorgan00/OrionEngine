#ifndef PLATFORM_H
#define PLATFORM_H

#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "resources/stb_image.h"

typedef struct platform_state {
  GLFWwindow* window;
} platform_state;

platform_state* platform_initialize();

GLFWwindow* platform_create_window();

stbi_uc* platform_open_image(const std::string filename, int* out_image_height,
                             int* out_image_width, int* out_channels);
void platform_shutdown();

std::vector<char> platform_read_file(const std::string& filename);

#endif
