#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "engine/renderer_types.inl"

VkShaderModule vulkan_shader_create(backend_context* context,
                                    const std::vector<char>& shader_code);
#endif
