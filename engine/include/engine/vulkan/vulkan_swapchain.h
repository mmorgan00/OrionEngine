#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "engine/renderer_types.inl"

void vulkan_swapchain_create(backend_context* context);

void vulkan_swapchain_create_image_views(backend_context* context);

#endif
