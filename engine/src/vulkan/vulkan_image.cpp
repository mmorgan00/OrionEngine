
#include "engine/vulkan/vulkan_image.h"

#include <vulkan/vulkan_core.h>

#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "engine/vulkan/vulkan_buffer.h"
#include "engine/vulkan/vulkan_command_buffer.h"

bool has_stencil_component(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}
/**
 * @brief creates a vulkan image sampler
 */
void vulkan_image_create_sampler(backend_context* context, vulkan_image* image,
                                 vk::Sampler* out_sampler) {
  vk::SamplerCreateInfo sampler_ci{
      .flags = vk::SamplerCreateFlags(),  // Ensure flags is placed here
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = context->device.properties.limits.maxSamplerAnisotropy,
      .compareEnable = VK_FALSE,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = VK_LOD_CLAMP_NONE,  // Use a proper value for maxLod if needed
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = VK_FALSE};
  *out_sampler = context->device.logical_device.createSampler(sampler_ci);
}

/**
 * @brief Creates a new vulkan image view and saves it into the provided
 * vulkan_image pointer
 * @param context - The vulkan context needed to issue vulkan commands
 * @param image - The vulkan_image to save the image view into
 */
void vulkan_image_create_view(backend_context* context, vk::Format format,
                              vk::ImageAspectFlagBits flags, vk::Image* image,
                              vk::ImageView* out_image_view) {
  vk::ImageViewCreateInfo view_ci{
      .image = *image,
      .viewType = vk::ImageViewType::e2D,
      .format = format,
      .components = {.r = vk::ComponentSwizzle::eIdentity,
                     .g = vk::ComponentSwizzle::eIdentity,
                     .b = vk::ComponentSwizzle::eIdentity,
                     .a = vk::ComponentSwizzle::eIdentity},
      .subresourceRange = {.aspectMask = flags,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1

      }};
  *out_image_view = context->device.logical_device.createImageView(view_ci);

  return;
}
void vulkan_image_copy_from_buffer(backend_context* context,
                                   vulkan_buffer buffer, vulkan_image image,
                                   uint32_t height, uint32_t width) {
  vk::CommandBuffer cmd_buf =
      vulkan_command_buffer_begin_single_time_commands(context);

  vk::BufferImageCopy region{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
      .imageOffset = {.x = 0, .y = 0, .z = 0},
      .imageExtent = {.width = width, .height = height, .depth = 1}};

  cmd_buf.copyBufferToImage(buffer.handle, image.handle,
                            vk::ImageLayout::eTransferDstOptimal, 1, &region);

  vulkan_command_buffer_end_single_time_commands(context, cmd_buf);
}

void vulkan_image_transition_layout(backend_context* context,
                                    vulkan_image* image, vk::Format format,
                                    vk::ImageLayout old_layout,
                                    vk::ImageLayout new_layout) {
  vk::CommandBuffer cmd_buf =
      vulkan_command_buffer_begin_single_time_commands(context);

  vk::ImageMemoryBarrier barrier{
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex =
          context->device.transfer_queue_index | vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex =
          context->device.transfer_queue_index | vk::QueueFamilyIgnored,
      .image = image->handle,
      .subresourceRange =
          {
              .aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = vk::RemainingMipLevels,
              .layerCount = 1,
          },
  };
  vk::PipelineStageFlags src_stage;
  vk::PipelineStageFlags dst_stage;
  // Set access masks
  if (old_layout == vk::ImageLayout::eUndefined &&
      new_layout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    dst_stage = vk::PipelineStageFlagBits::eTransfer;

  } else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    src_stage = vk::PipelineStageFlagBits::eTransfer;
    dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
  } else if (old_layout == vk::ImageLayout::eUndefined &&
             new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
  } else {
    throw std::invalid_argument("Invalid transition");
  }

  if (new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (has_stencil_component(format)) {
      barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  cmd_buf.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(),
                          static_cast<uint32_t>(0), nullptr,
                          static_cast<uint32_t>(0), nullptr, 1, &barrier);

  vulkan_command_buffer_end_single_time_commands(context, cmd_buf);
}

void vulkan_image_create(backend_context* context, uint32_t height,
                         uint32_t width, vk::Format format,
                         vk::Flags<vk::ImageUsageFlagBits> usage,
                         vulkan_image* out_image) {
  vk::ImageCreateInfo image_ci{
      .flags = vk::ImageCreateFlags(),
      .imageType = vk::ImageType::e2D,
      .format = format,
      .extent =
          {
              .width = width,
              .height = height,
              .depth = 1,
          },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined,
  };

  out_image->handle = context->device.logical_device.createImage(image_ci);

  // Memory now
  vk::MemoryRequirements mem_reqs =
      context->device.logical_device.getImageMemoryRequirements(
          out_image->handle);

  vk::MemoryAllocateInfo alloc_info{
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex =
          find_memory_type(context, mem_reqs.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eDeviceLocal),
  };

  out_image->memory = context->device.logical_device.allocateMemory(alloc_info);

  context->device.logical_device.bindImageMemory(out_image->handle,
                                                 out_image->memory, 0);
}
