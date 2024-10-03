
#include "engine/vulkan/vulkan_image.h"

#include <vulkan/vulkan_core.h>

#include <stdexcept>

#include "engine/vulkan/vulkan_buffer.h"
#include "engine/vulkan/vulkan_command_buffer.h"

void vulkan_image_create_sampler(backend_context* context, vulkan_image* image,
                                 VkSampler* out_sampler) {
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_TRUE;
  sampler_info.maxAnisotropy =
      context->device.properties.limits.maxSamplerAnisotropy;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  VK_CHECK(vkCreateSampler(context->device.logical_device, &sampler_info,
                           nullptr, out_sampler));
}

/**
 * @brief Creates a new vulkan image view and saves it into the provided
 * vulkan_image pointer
 * @param context - The vulkan context needed to issue vulkan commands
 * @param image - The vulkan_image to save the image view into
 */
void vulkan_image_create_view(backend_context* context, VkFormat format,
                              VkImage* image, VkImageView* out_image_view) {
  VkImageViewCreateInfo view_ci{};
  view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_ci.image = *image;
  view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_ci.format = format;  // VK_FORMAT_R8G8B8A8_SRGB;
  view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_ci.subresourceRange.baseMipLevel = 0;
  view_ci.subresourceRange.levelCount = 1;
  view_ci.subresourceRange.baseArrayLayer = 0;
  view_ci.subresourceRange.layerCount = 1;
  VK_CHECK(vkCreateImageView(context->device.logical_device, &view_ci, nullptr,
                             out_image_view));
  return;
}
void vulkan_image_copy_from_buffer(backend_context* context,
                                   vulkan_buffer buffer, vulkan_image image,
                                   uint32_t height, uint32_t width) {
  VkCommandBuffer cmd_buf =
      vulkan_command_buffer_begin_single_time_commands(context);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(cmd_buf, buffer.handle, image.handle,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vulkan_command_buffer_end_single_time_commands(context, cmd_buf);
}

void vulkan_image_transition_layout(backend_context* context,
                                    vulkan_image* image, VkFormat format,
                                    VkImageLayout old_layout,
                                    VkImageLayout new_layout) {
  VkCommandBuffer cmd_buf =
      vulkan_command_buffer_begin_single_time_commands(context);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex =
      context->device.transfer_queue_index | VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex =
      context->device.transfer_queue_index | VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image->handle;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;

  VkPipelineStageFlags src_stage;
  VkPipelineStageFlags dst_stage;
  // Set access masks
  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }
  vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr,
                       1, &barrier);

  vulkan_command_buffer_end_single_time_commands(context, cmd_buf);
}

void vulkan_image_create(backend_context* context, uint32_t height,
                         uint32_t width, vulkan_image* out_image) {
  VkImageCreateInfo image_ci{};
  image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_ci.imageType = VK_IMAGE_TYPE_2D;
  image_ci.extent.width = width;
  image_ci.extent.height = height;
  image_ci.extent.depth = 1;
  image_ci.mipLevels = 1;
  image_ci.arrayLayers = 1;
  image_ci.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
  image_ci.flags = 0;

  VK_CHECK(vkCreateImage(context->device.logical_device, &image_ci, nullptr,
                         &out_image->handle));

  // Memory now
  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(context->device.logical_device,
                               out_image->handle, &mem_reqs);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = find_memory_type(
      context, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_CHECK(vkAllocateMemory(context->device.logical_device, &alloc_info,
                            nullptr, &out_image->memory));

  vkBindImageMemory(context->device.logical_device, out_image->handle,
                    out_image->memory, 0);
}
