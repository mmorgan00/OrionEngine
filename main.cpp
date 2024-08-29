
#define GLFW_INCLUDE_VULKAN

#include <engine/application.h>
#include <engine/renderer.h>

#include <stdio.h>
#include <iostream>


int main() {
  platform_initialize();
  renderer_initialize();
  while(application_run()) {
  }
  renderer_shutdown();
  platform_shutdown();
}
