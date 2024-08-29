#include "engine/renderer.h"

#include "engine/renderer_backend.h"
#include "engine/logger.h"

bool renderer_initialize() {
    
  if(renderer_backend_initialize()){
    OE_LOG(LOG_LEVEL_INFO, "Renderer initialized");
    return true;
  }
  OE_LOG(LOG_LEVEL_FATAL, "Error initializing renderer");
  return false;
  
}


void renderer_shutdown() {
  renderer_backend_shutdown();
}
