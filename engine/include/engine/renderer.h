#ifndef RENDERER_H
#define RENDERER_H

#include "engine/platform.h"

bool renderer_initialize(platform_state* plat_state);

/**
 * @brief Loads an object to be rendered
 */
void load_object();

void draw_frame();
void renderer_shutdown();

#endif
