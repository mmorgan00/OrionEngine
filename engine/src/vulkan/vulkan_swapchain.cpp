#include "engine/vulkan/vulkan_swapchain.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>  // Necessary for std::clamp
#include <limits>     // Necessary for std::numeric_limits
#include <vector>

#include "engine/logger.h"
#include "engine/renderer_types.inl"
#include "engine/vulkan/vulkan_image.h"

VkSurfaceFormatKHR swapchain_select_surface_format(
    const std::vector<VkSurfaceFormatKHR> available_formats) {
  for (const auto &format : available_formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }

  return available_formats[0];
}

VkPresentModeKHR swapchain_select_present_mode(
    uint32_t present_mode_count, VkPresentModeKHR *available_present_modes) {
  for (int i = 0; i < present_mode_count; i++) {
    if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      OE_LOG(LOG_LEVEL_INFO, "Found mailbox present mode, selecting...");
      return available_present_modes[i];
    }
  }
  // fallback
  OE_LOG(LOG_LEVEL_INFO, "Mailbox present mode not found. Defaulting to FIFO");
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D swapchain_select_extent(
    GLFWwindow *window, const VkSurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void create(backend_context *context) {
  // Going to reference this a lot, so for convenience sake
  auto sc_s = context->device.swapchain_support;

  VkSurfaceFormatKHR surface_format =
      swapchain_select_surface_format(sc_s.formats);

  VkPresentModeKHR present_mode =
      swapchain_select_present_mode(sc_s.format_count, sc_s.present_modes);

  VkExtent2D extent =
      swapchain_select_extent(context->window, sc_s.capabilities);

  uint32_t image_count = sc_s.capabilities.minImageCount + 1;

  // Make sure we don't have too many images
  if (sc_s.capabilities.maxImageCount > 0 &&
      image_count > sc_s.capabilities.maxImageCount) {
    image_count = sc_s.capabilities.maxImageCount;
  }

  // Everyone's favorite thing to do in vulkan - fill out a struct for
  // instantiation
  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = context->surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  // TODO: WHEN WE GET TO DEFFERED RENDERING, REVIST THIS
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (context->device.graphics_queue_index !=
      context->device.present_queue_index) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    uint32_t queue_family_indices[] = {
        static_cast<uint32_t>(context->device.graphics_queue_index),
        static_cast<uint32_t>(context->device.present_queue_index)};
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = nullptr;
  }

  create_info.preTransform = sc_s.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;  // Clips pixels behind other windows.
  create_info.oldSwapchain =
      VK_NULL_HANDLE;  // Only a 'recreate swapchain' type of thing

  VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device, &create_info,
                                nullptr, &context->swapchain.handle));

  // Save handles to the swap chain images
  vkGetSwapchainImagesKHR(context->device.logical_device,
                          context->swapchain.handle, &image_count, nullptr);
  context->swapchain.images.resize(image_count);
  context->swapchain.image_count = image_count;
  vkGetSwapchainImagesKHR(context->device.logical_device,
                          context->swapchain.handle, &image_count,
                          context->swapchain.images.data());

  OE_LOG(LOG_LEVEL_DEBUG, "Retrieved swapchain images");
  // We'll want these later
  context->swapchain.image_format = surface_format.format;
  context->swapchain.extent = extent;
}

void vulkan_swapchain_destroy(backend_context *context) {
  for (size_t i = 0; i < context->swapchain.framebuffers.size(); i++) {
    if (context->swapchain.framebuffers[i] != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(context->device.logical_device,
                           context->swapchain.framebuffers[i], nullptr);
    }
  }

  for (size_t i = 0; i < context->swapchain.views.size(); i++) {
    if (context->swapchain.views[i] != VK_NULL_HANDLE) {
      vkDestroyImageView(context->device.logical_device,
                         context->swapchain.views[i], nullptr);
    }
  }

  vkDestroySwapchainKHR(context->device.logical_device,
                        context->swapchain.handle, nullptr);
}

void recreate_swapchain(backend_context *context) {
  vkDeviceWaitIdle(context->device.logical_device);

  vulkan_swapchain_destroy(context);
  create(context);
}

void vulkan_swapchain_create(backend_context *context) {
  create(context);  // just call helper function
  OE_LOG(LOG_LEVEL_INFO, "Swapchain created!");
}

void vulkan_swapchain_create_image_views(backend_context *context) {
  context->swapchain.views.resize(context->swapchain.image_count);

  for (size_t i = 0; i < context->swapchain.image_count; i++) {
    vulkan_image_create_view(context, context->swapchain.image_format,
                             &context->swapchain.images[i],
                             &context->swapchain.views[i]);
  }
  OE_LOG(LOG_LEVEL_DEBUG, "Swapchain image views created");
}
