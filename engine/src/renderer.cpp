#include "engine/renderer.h"

#include "engine/renderer_backend.h"

//TODO: remove
#include <stdio.h>
#include <iostream>

void renderer_initialize() {
    
  if(renderer_backend_initialize()){
    std::cout << "Renderer initialized" << std::endl;
    return;
  }
  std::cout << "Error initializing renderer" << std::endl;
  
}
