#include "engine/vulkan/vulkan_swapchain.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>  // Necessary for std::clamp
#include <limits>     // Necessary for std::numeric_limits
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "engine/logger.h"
#include "engine/renderer_types.inl"
#include "engine/vulkan/vulkan_image.h"

vk::SurfaceFormatKHR swapchain_select_surface_format(
    const std::vector<vk::SurfaceFormatKHR> available_formats) {
  for (const auto &format : available_formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }

  return available_formats[0];
}

vk::PresentModeKHR swapchain_select_present_mode(
    uint32_t present_mode_count, vk::PresentModeKHR *available_present_modes) {
  for (int i = 0; i < present_mode_count; i++) {
    if (available_present_modes[i] == vk::PresentModeKHR::eMailbox) {
      OE_LOG(LOG_LEVEL_INFO, "Found mailbox present mode, selecting...");
      return available_present_modes[i];
    }
  }
  // fallback
  OE_LOG(LOG_LEVEL_INFO, "Mailbox present mode not found. Defaulting to FIFO");
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D swapchain_select_extent(
    GLFWwindow *window, const vk::SurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actual_extent = {static_cast<uint32_t>(width),
                                  static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
  }
}

void create(backend_context *context) {
  // Going to reference this a lot, so for convenience sake
  auto sc_s = context->device.swapchain_support;

  vk::SurfaceFormatKHR surface_format =
      swapchain_select_surface_format(sc_s.formats);

  vk::PresentModeKHR present_mode =
      swapchain_select_present_mode(sc_s.format_count, sc_s.present_modes);

  vk::Extent2D extent =
      swapchain_select_extent(context->window, sc_s.capabilities);

  uint32_t image_count = sc_s.capabilities.minImageCount + 1;

  // Make sure we don't have too many images
  if (sc_s.capabilities.maxImageCount > 0 &&
      image_count > sc_s.capabilities.maxImageCount) {
    image_count = sc_s.capabilities.maxImageCount;
  }

  // Everyone's favorite thing to do in vulkan - fill out a struct for
  // instantiation
  std::array<uint32_t, 2> queue_family_indices = {
      static_cast<uint32_t>(context->device.graphics_queue_index),
      static_cast<uint32_t>(context->device.present_queue_index)};

  // If graphics = present
  vk::SharingMode sharing_mode;
  if (queue_family_indices[0] != queue_family_indices[1]) {
    sharing_mode = vk::SharingMode::eConcurrent;
  } else {
    sharing_mode = vk::SharingMode::eExclusive;
    queue_family_indices = {};
  }
  vk::SwapchainCreateInfoKHR sc_ci{
      .flags = {},
      .surface = context->surface,
      .minImageCount = 2,
      .imageFormat = vk::Format::eB8G8R8A8Srgb,
      .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = sharing_mode,
      .queueFamilyIndexCount = queue_family_indices.size(),
      .pQueueFamilyIndices = queue_family_indices.data(),
      .preTransform = sc_s.capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = vk::PresentModeKHR::eFifo,
      .clipped = VK_TRUE,
      .oldSwapchain = nullptr};

  context->swapchain.handle =
      context->device.logical_device.createSwapchainKHR(sc_ci);

  context->swapchain.images =
      context->device.logical_device.getSwapchainImagesKHR(
          context->swapchain.handle);

  context->swapchain.image_count = context->swapchain.images.size();
  // vkGetSwapchainImagesKHR(context->device.logical_device,
  //                         context->swapchain.handle, &image_count,
  //                         context->swapchain.images.data());

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
