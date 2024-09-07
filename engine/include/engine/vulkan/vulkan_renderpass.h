#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "engine/renderer_types.inl"

void vulkan_renderpass_create(backend_context* context,
                              vulkan_renderpass out_renderpass);
#endif
