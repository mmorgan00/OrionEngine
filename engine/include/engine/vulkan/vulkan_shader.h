#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <string>

#include "engine/renderer_types.inl"

vulkan_object_shader vulkan_shader_create(backend_context* context,
                                          const std::string vert_path,
                                          const std::string frag_path);
#endif
