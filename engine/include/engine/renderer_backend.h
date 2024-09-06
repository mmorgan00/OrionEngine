#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "engine/asserts.h"
#include "engine/platform.h"
#include "engine/renderer_types.inl"

bool renderer_backend_initialize(platform_state* plat_state);

void renderer_backend_shutdown();

#endif
