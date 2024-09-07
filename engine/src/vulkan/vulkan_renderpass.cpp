#include "engine/vulkan/vulkan_renderpass.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "engine/renderer_types.inl"

void vulkan_renderpass_create(backend_context* context,
                              vulkan_renderpass out_renderpass) {
  VkAttachmentDescription color_attachment{};

  color_attachment.format = context->swapchain.image_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;  // Not doing multisampling
  color_attachment.loadOp =
      VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear to black before drawing.
  color_attachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE;  // Other option is undefined, and we want
                                     // to see stuff, sooo
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // Dont know what comes in
  color_attachment.finalLayout =
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // We want to present what comes out

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Just one subpass for now. We'll revist for post-processing
  VkSubpassDescription subpass{};
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkRenderPassCreateInfo renderpass_create_info{};
  renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass_create_info.attachmentCount = 1;
  renderpass_create_info.pAttachments = &color_attachment;
  renderpass_create_info.subpassCount = 1;
  renderpass_create_info.pSubpasses = &subpass;

  VK_CHECK(vkCreateRenderPass(context->device.logical_device,
                              &renderpass_create_info, nullptr,
                              &out_renderpass.handle));
}
