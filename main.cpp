
#define GLFW_INCLUDE_VULKAN

#include <engine/application.h>
#include <engine/platform.h>
#include <engine/renderer.h>

int main() {
  platform_state *plat_state = platform_initialize();
  application_initialize(plat_state);
  renderer_initialize(plat_state);
  while (application_run());
  application_shutdown();
  platform_shutdown();
  plat_state = 0;
}
