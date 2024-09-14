#include "engine/vulkan/vulkan_buffer.h"

#include <vulkan/vulkan_core.h>

#include <cstring>
#include <stdexcept>

uint32_t find_memory_type(backend_context* context, uint32_t type_filter,
                          VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(context->device.physical_device,
                                      &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags &
                                   properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory for buffer");
}

void vulkan_buffer_create(backend_context* context, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          std::vector<Vertex> vertices,
                          vulkan_buffer* out_buffer) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = sizeof(vertices[0]) * vertices.size();
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, nullptr,
                          &out_buffer->handle));

  // Memory requirements
  VkMemoryRequirements mem_reqs{};
  vkGetBufferMemoryRequirements(context->device.logical_device,
                                out_buffer->handle, &mem_reqs);
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(context, mem_reqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK(vkAllocateMemory(context->device.logical_device, &alloc_info,
                            nullptr, &out_buffer->memory));

  vkBindBufferMemory(context->device.logical_device, *out_buffer, buffer_memory,
                     0);

  // map memory
  void* data;
  vkMapMemory(context->device.logical_device, buffer_memory, 0,
              buffer_info.size, 0, &data);
  memcpy(data, vertices.data(), (size_t)buffer_info.size);
  vkUnmapMemory(context->device.logical_device, buffer_memory);
  vkBindBufferMemory(context->device.logical_device, out_buffer->handle,
                     out_buffer->memory, 0);
}

void vulkan_buffer_load_data(backend_context* context, vulkan_buffer* buffer,
                             long offset, long size, uint32_t flags,
                             const void* data) {
  void* data_ptr;
  vkMapMemory(context->device.logical_device, buffer->memory, 0, size, 0,
              &data_ptr);
  memcpy(data_ptr, data, size);
  vkUnmapMemory(context->device.logical_device, buffer->memory);
}
