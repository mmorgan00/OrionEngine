#include "engine/vulkan/vulkan_command_buffer.h"

VkCommandBuffer vulkan_command_buffer_begin_single_time_commands(
    backend_context* context) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = context->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(context->device.logical_device, &alloc_info,
                           &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

void vulkan_command_buffer_end_single_time_commands(
    backend_context* context, VkCommandBuffer command_buffer) {
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
}
