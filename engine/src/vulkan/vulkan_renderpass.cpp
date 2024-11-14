#include "engine/vulkan/vulkan_renderpass.h"

#include <engine/renderer_backend.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "engine/renderer_types.inl"

void vulkan_renderpass_create(backend_context* context,
                              vulkan_renderpass* out_renderpass) {
  vk::AttachmentDescription depth_attachment{
      .format = find_depth_format(),
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::AttachmentDescription color_attachment{
      .format = context->swapchain.image_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR};

  vk::AttachmentReference depth_attachment_ref{
      .attachment = 1,
      .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::AttachmentReference color_attachment_ref{
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

  vk::SubpassDependency subpass_dependency{
      .srcSubpass = vk::SubpassExternal,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                      vk::PipelineStageFlagBits::eEarlyFragmentTests,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                      vk::PipelineStageFlagBits::eEarlyFragmentTests,
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                       vk::AccessFlagBits::eDepthStencilAttachmentWrite};
  //
  // Just one subpass for now. We'll revist for post-processing
  vk::SubpassDescription subpass{
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref,
      .pDepthStencilAttachment = &depth_attachment_ref,
  };

  std::array<vk::AttachmentDescription, 2> attachments = {color_attachment,
                                                          depth_attachment};

  vk::RenderPassCreateInfo renderpass_ci{.attachmentCount = attachments.size(),
                                         .pAttachments = attachments.data(),
                                         .subpassCount = 1,
                                         .pSubpasses = &subpass,
                                         .dependencyCount = 1,
                                         .pDependencies = &subpass_dependency};
  context->main_renderpass.handle =
      context->device.logical_device.createRenderPass(renderpass_ci);
}
