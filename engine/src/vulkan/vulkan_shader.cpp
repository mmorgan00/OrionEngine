#include "engine/vulkan/vulkan_shader.h"

#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>

#include "engine/logger.h"
#include "engine/platform.h"
#include "engine/renderer_types.inl"
#include "engine/vulkan/vulkan_pipeline.h"

vulkan_object_shader vulkan_shader_create(backend_context* context,
                                          vulkan_renderpass* renderpass,
                                          const std::string vert_path,
                                          const std::string frag_path) {
  std::string asset_path = "../assets/shaders/";
  vulkan_object_shader vos{};
  // Read in shaders code
  auto vert_shader_code = platform_read_file(asset_path + vert_path);
  auto frag_shader_code = platform_read_file(asset_path + frag_path);
  OE_LOG(LOG_LEVEL_INFO, "Default shaders created");

  vos.stages[0].create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  vos.stages[0].create_info.codeSize = vert_shader_code.size();
  vos.stages[0].create_info.pCode =
      reinterpret_cast<const uint32_t*>(vert_shader_code.data());

  VK_CHECK(vkCreateShaderModule(context->device.logical_device,
                                &vos.stages[0].create_info, nullptr,
                                &vos.stages[0].handle));

  vos.stages[1].create_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  vos.stages[1].create_info.codeSize = vert_shader_code.size();
  vos.stages[1].create_info.pCode =
      reinterpret_cast<const uint32_t*>(frag_shader_code.data());

  VK_CHECK(vkCreateShaderModule(context->device.logical_device,
                                &vos.stages[1].create_info, nullptr,
                                &vos.stages[1].handle));

  vulkan_pipeline_create(context, &context->main_renderpass,
                         vos.stages[0].handle, vos.stages[1].handle,
                         &context->pipeline);

  return vos;
}
