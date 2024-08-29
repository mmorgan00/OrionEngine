

#define GLFW_INCLUDE_VULKAN

#include "engine/platform.h"
#include "engine/application.h"
#include "engine/logger.h"

#include <stdio.h>
#include <iostream>


static platform_state* plat_state;

bool platform_initialize(){
  plat_state = (platform_state*)malloc(sizeof(platform_state));

  application_initialize(plat_state);
  if(!glfwInit()){
    return -1;
  };


  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWmonitor* primary = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(primary);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  plat_state->window = glfwCreateWindow(mode->width, mode->height, "Orion", primary, NULL);

  if(!plat_state->window){
    glfwTerminate();
    return -1;
  }
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  OE_LOG(LOG_LEVEL_INFO, "Platform Initialized!");

  /*while(!glfwWindowShouldClose(window))*/
  /*{*/
  /*  glfwPollEvents();*/
  /*}*/
  /**/

  return true;
}


void platform_shutdown() {
  glfwDestroyWindow(plat_state->window);

  glfwTerminate();
}
