#include "engine/renderer.h"

#include "engine/logger.h"
#include "engine/renderer_backend.h"

bool renderer_initialize(platform_state *plat_state) {
  if (renderer_backend_initialize(plat_state)) {
    OE_LOG(LOG_LEVEL_INFO, "Renderer initialized");
    return true;
  }
  OE_LOG(LOG_LEVEL_FATAL, "Error initializing renderer");
  return false;
}

void load_object() { renderer_backend_load_geometry(); }

void draw_frame() {
  // TODO: call draw for each object, swap textures, shaders, materials, etc
  // etc. LOTS will happen here
  renderer_backend_draw_frame();
}
void renderer_shutdown() { renderer_backend_shutdown(); }
