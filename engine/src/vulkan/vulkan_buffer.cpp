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

void vulkan_buffer_create(backend_context* context,
                          std::vector<Vertex> vertices, VkBuffer* out_buffer) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = sizeof(vertices[0]) * vertices.size();
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, nullptr,
                          out_buffer));

  // Memory requirements
  VkDeviceMemory buffer_memory;
  VkMemoryRequirements mem_reqs{};
  vkGetBufferMemoryRequirements(context->device.logical_device, *out_buffer,
                                &mem_reqs);
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(context, mem_reqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK(vkAllocateMemory(context->device.logical_device, &alloc_info,
                            nullptr, &buffer_memory));

  vkBindBufferMemory(context->device.logical_device, *out_buffer, buffer_memory,
                     0);

  // map memory
  void* data;
  vkMapMemory(context->device.logical_device, buffer_memory, 0,
              buffer_info.size, 0, &data);
  memcpy(data, vertices.data(), (size_t)buffer_info.size);
  vkUnmapMemory(context->device.logical_device, buffer_memory);
}
