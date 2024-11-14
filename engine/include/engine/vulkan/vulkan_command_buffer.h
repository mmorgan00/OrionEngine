#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include "engine/renderer_types.inl"

VkCommandBuffer vulkan_command_buffer_begin_single_time_commands(
    backend_context* context);

void vulkan_command_buffer_end_single_time_commands(
    backend_context* context, VkCommandBuffer command_buffer);
#endif
