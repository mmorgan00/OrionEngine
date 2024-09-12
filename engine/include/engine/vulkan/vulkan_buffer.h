#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <vulkan/vulkan_core.h>

#include <vector>

#include "engine/renderer_types.inl"

void vulkan_buffer_create(backend_context* context,
                          std::vector<Vertex> vertices, VkBuffer* out_buffer);

#endif
