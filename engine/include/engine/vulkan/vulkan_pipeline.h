#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "engine/renderer_types.inl"

void vulkan_pipeline_create(backend_context *context,
                            VkShaderModule vert_shader,
                            VkShaderModule frag_shader,
                            vulkan_pipeline out_pipeline);
#endif
