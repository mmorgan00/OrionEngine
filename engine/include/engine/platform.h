#ifndef PLATFORM_H
#define PLATFORM_H

#include <GLFW/glfw3.h>

typedef struct platform_state {

  GLFWwindow* window;
} platform_state;

bool platform_initialize();


void platform_shutdown();



#endif
