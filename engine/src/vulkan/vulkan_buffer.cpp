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

void vulkan_buffer_load_data(backend_context* context, vulkan_buffer* buffer,
                             long offset, uint32_t flags, long size,
                             const void* buff_data) {
  void* data;
  vkMapMemory(context->device.logical_device, buffer->memory, 0, size, 0,
              &data);
  memcpy(data, buff_data, (size_t)size);
  vkUnmapMemory(context->device.logical_device, buffer->memory);
}
void vulkan_buffer_copy(backend_context* context, vulkan_buffer* source,
                        vulkan_buffer* target, VkDeviceSize size) {
  // Going to use a one time command buffer to copy and immediately start
  // recording
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = context->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(context->device.logical_device, &alloc_info,
                           &command_buffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &beginInfo);

  // Perform the copy
  VkBufferCopy copy_info{};
  copy_info.srcOffset = 0;
  copy_info.dstOffset = 0;
  copy_info.size = size;

  vkCmdCopyBuffer(command_buffer, source->handle, target->handle, 1,
                  &copy_info);
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(context->device.graphics_queue, 1, &submit_info,
                VK_NULL_HANDLE);
  vkQueueWaitIdle(context->device.graphics_queue);

  vkFreeCommandBuffers(context->device.logical_device, context->command_pool, 1,
                       &command_buffer);
  return;
}
void vulkan_buffer_create(backend_context* context, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkDeviceSize size,
                          vulkan_buffer* out_buffer) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
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

  vkBindBufferMemory(context->device.logical_device, out_buffer->handle,
                     out_buffer->memory, 0);

  // TODO: From here down should be a separate function call
  // map memory
  // void* data;
  // vkMapMemory(context->device.logical_device, out_buffer->memory, 0,
  //             buffer_info.size, 0, &data);
  // memcpy(data, vertices.data(), (size_t)buffer_info.size);
  // vkUnmapMemory(context->device.logical_device, out_buffer->memory);
}
