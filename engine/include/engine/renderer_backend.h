#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "engine/platform.h"

bool renderer_backend_initialize(platform_state* plat_state);

void renderer_create_texture();

void renderer_backend_draw_frame();
void renderer_backend_draw_image(uint32_t image_index);

void renderer_backend_shutdown();

#endif
