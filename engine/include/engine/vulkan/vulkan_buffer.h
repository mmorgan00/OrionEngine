#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <vulkan/vulkan_core.h>

#include <vector>

#include "engine/renderer_types.inl"

void vulkan_buffer_create(backend_context* context, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          std::vector<Vertex> vertices,
                          vulkan_buffer* out_buffer);

void vulkan_buffer_load_data(backend_context* context, vulkan_buffer* buffer,
                             long offset, uint32_t flags, long size,
                             const void* buff_data);

#endif
