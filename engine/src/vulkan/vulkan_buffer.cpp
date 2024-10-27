#include "engine/vulkan/vulkan_buffer.h"

#include <cstring>
#include <stdexcept>

#include "engine/vulkan/vulkan_command_buffer.h"

uint32_t find_memory_type(backend_context* context, uint32_t type_filter,
                          vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties mem_properties =
      context->device.physical_device.getMemoryProperties();

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags &
                                   properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory for buffer");
}

void vulkan_buffer_destroy(backend_context* context, vulkan_buffer* buffer) {
  context->device.logical_device.destroyBuffer(buffer->handle, nullptr);
  context->device.logical_device.freeMemory(buffer->memory);
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
                        vulkan_buffer* target, vk::DeviceSize size) {
  VkCommandBuffer command_buffer =
      vulkan_command_buffer_begin_single_time_commands(context);
  // // Perform the copy
  VkBufferCopy copy_info{};
  copy_info.srcOffset = 0;
  copy_info.dstOffset = 0;
  copy_info.size = size;

  vkCmdCopyBuffer(command_buffer, source->handle, target->handle, 1,
                  &copy_info);

  vulkan_command_buffer_end_single_time_commands(context, command_buffer);

  return;
}
void vulkan_buffer_create(backend_context* context, vk::BufferUsageFlags usage,
                          vk::MemoryPropertyFlags properties,
                          vk::DeviceSize size, vulkan_buffer* out_buffer) {
  vk::BufferCreateInfo buffer_ci{
      .size = size,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
  };
  out_buffer->handle = context->device.logical_device.createBuffer(buffer_ci);

  // Memory requirements
  vk::MemoryRequirements mem_reqs =
      context->device.logical_device.getBufferMemoryRequirements(
          out_buffer->handle);

  auto mem_type_index =
      find_memory_type(context, mem_reqs.memoryTypeBits, properties);
  vk::MemoryAllocateInfo alloc_info{
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex = mem_type_index,
  };
  out_buffer->memory =
      context->device.logical_device.allocateMemory(alloc_info);

  context->device.logical_device.bindBufferMemory(out_buffer->handle,
                                                  out_buffer->memory, 0);
}
