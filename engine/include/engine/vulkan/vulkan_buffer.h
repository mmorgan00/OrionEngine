#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include "engine/renderer_types.inl"

void vulkan_buffer_create(backend_context* context, vk::BufferUsageFlags usage,
                          vk::MemoryPropertyFlags properties,
                          vk::DeviceSize size, vulkan_buffer* out_buffer);

void vulkan_buffer_destroy(backend_context* context, vulkan_buffer* buffer);

void vulkan_buffer_copy(backend_context* context, vulkan_buffer* source,
                        vulkan_buffer* target, vk::DeviceSize size);

void vulkan_buffer_load_data(backend_context* context, vulkan_buffer* buffer,
                             long offset, uint32_t flags, long size,
                             const void* buff_data);

uint32_t find_memory_type(backend_context* context, uint32_t type_filter,
                          vk::MemoryPropertyFlags properties);

#endif
