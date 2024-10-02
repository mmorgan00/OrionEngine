
#define GLFW_INCLUDE_VULKAN

#include <engine/application.h>
#include <engine/platform.h>
#include <engine/renderer.h>

int main() {
  platform_state *plat_state = platform_initialize();
  renderer_initialize(plat_state);
  application_initialize(plat_state);
  while (application_run()) {
  }
  renderer_shutdown();
  platform_shutdown();
  plat_state = 0;
}
