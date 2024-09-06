#include "engine/vulkan/vulkan_shader.h"

#include <vulkan/vulkan_core.h>

#include <vector>

VkShaderModule vulkan_shader_create(backend_context* context,
                                    const std::vector<char>& shader_code) {
  VkShaderModuleCreateInfo shader_create_info{};
  shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  shader_create_info.codeSize = shader_code.size();
  shader_create_info.pCode =
      reinterpret_cast<const uint32_t*>(shader_code.data());

  VkShaderModule shader;
  VK_CHECK(vkCreateShaderModule(context->device.logical_device,
                                &shader_create_info, nullptr, &shader));
  return shader;
}
