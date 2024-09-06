#ifndef PLATFORM_H
#define PLATFORM_H

#include <GLFW/glfw3.h>

#include <fstream>
#include <vector>

typedef struct platform_state {
  GLFWwindow* window;
} platform_state;

platform_state* platform_initialize();

GLFWwindow* platform_create_window();

void platform_shutdown();

std::vector<char> platform_read_file(const std::string& filename);

#endif
