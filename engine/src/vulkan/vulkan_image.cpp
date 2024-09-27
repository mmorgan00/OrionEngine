
#include "engine/vulkan/vulkan_image.h"

#include <vulkan/vulkan_core.h>

#include "engine/platform.h"
#include "engine/vulkan/vulkan_buffer.h"

void vulkan_image_create(backend_context* context, vulkan_image* out_image) {
  int width, height, channels;
  void* pixels;
  platform_open_image("resources/default", &width, &height, &channels, &pixels);

  vulkan_buffer staging;
  vulkan_buffer_create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       width * height * 4, &staging);

  vulkan_buffer_load_data(context, &staging, 0, 0, width * height * 4, pixels);
  stbi_image_free(pixels);

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
