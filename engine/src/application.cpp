
#include "engine/application.h"


#define GLFW_INCLUDE_VULKAN
 
#include <stdio.h>
#include <iostream>


#include <GLFW/glfw3.h>

static platform_state* plat_state;

void application_initialize(platform_state* state) {
  plat_state = state;
}

bool application_run() {

  while(!glfwWindowShouldClose(plat_state->window)) {
    glfwPollEvents();
  }
  return true;
}
