#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "engine/asserts.h"
#include "engine/renderer_types.inl"

#include <vulkan/vulkan.h>

bool renderer_backend_initialize();

void renderer_backend_shutdown();

#endif
