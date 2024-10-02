#include "engine/vulkan/vulkan_shader.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <vector>

#include "engine/filesystem.h"
#include "engine/logger.h"
#include "engine/platform.h"
#include "engine/renderer_types.inl"
#include "engine/vulkan/vulkan_pipeline.h"

vulkan_object_shader vulkan_shader_create(backend_context* context,
                                          vulkan_renderpass* renderpass,
                                          const std::string vert_path,
                                          const std::string frag_path) {
  std::string asset_path = "../bin/assets/shaders/";
  std::string vert_code = asset_path + vert_path;
  std::string frag_code = asset_path + frag_path;

  vulkan_object_shader vos{};
  // Read in shaders code
  file_handle handle;
  if (!filesystem_open(vert_code.c_str(), FILE_MODE_READ, true, &handle)) {
    OE_LOG(LOG_LEVEL_ERROR, "Unable to read shader module: %s.",
           vert_code.c_str());
  }

  // Read file in entirety
  long size = 0;
  char* file_buffer = 0;
  if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {
    OE_LOG(LOG_LEVEL_ERROR, "Unable to binary read shader module: %s.",
           vert_path.c_str());
  }

  // OE_LOG(LOG_LEVEL_INFO, "Default shaders created");

  vos.stages[0].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vos.stages[0].create_info.codeSize = size;
  vos.stages[0].create_info.pCode = (uint32_t*)file_buffer;

  VK_CHECK(vkCreateShaderModule(context->device.logical_device,
                                &vos.stages[0].create_info, nullptr,
                                &vos.stages[0].handle));

  std::vector<char> frag_shader_code =
      platform_read_file(asset_path + frag_path);
  if (!filesystem_open(frag_code.c_str(), FILE_MODE_READ, true, &handle)) {
    OE_LOG(LOG_LEVEL_ERROR, "Unable to read shader module: %s.",
           frag_code.c_str());
  }
  size = 0;
  file_buffer = 0;
  if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {
    OE_LOG(LOG_LEVEL_ERROR, "Unable to binary read shader module: %s.",
           frag_path.c_str());
  }

  vos.stages[1].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vos.stages[1].create_info.codeSize = size;
  vos.stages[1].create_info.pCode = (uint32_t*)file_buffer;

  VK_CHECK(vkCreateShaderModule(context->device.logical_device,
                                &vos.stages[1].create_info, nullptr,
                                &vos.stages[1].handle));

  vulkan_pipeline_create(context, &context->main_renderpass,
                         vos.stages[0].handle, vos.stages[1].handle,
                         &context->pipeline);

  return vos;
}
