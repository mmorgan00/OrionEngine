#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "engine/platform.h"
#include "engine/renderer_types.inl"

bool renderer_backend_initialize(platform_state* plat_state);

void renderer_create_texture();

vk::Format find_depth_format();

void renderer_backend_draw_frame();
void renderer_backend_draw_image(uint32_t image_index);

void renderer_backend_shutdown();

#endif
