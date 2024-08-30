#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "engine/asserts.h"
#include "engine/renderer_types.inl"
#include "engine/platform.h"

#include <vulkan/vulkan.h>

bool renderer_backend_initialize(platform_state* plat_state);

void renderer_backend_shutdown();

#endif
