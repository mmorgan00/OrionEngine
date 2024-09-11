#ifndef RENDERER_H
#define RENDERER_H

#include "engine/platform.h"

bool renderer_initialize(platform_state* plat_state);
void draw_frame();
void renderer_shutdown();

#endif
