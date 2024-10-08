
#include "engine/renderer_backend.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "engine/logger.h"
#include "engine/platform.h"
#include "engine/vulkan/vulkan_device.h"
#include "engine/vulkan/vulkan_pipeline.h"
#include "engine/vulkan/vulkan_renderpass.h"
#include "engine/vulkan/vulkan_shader.h"
#include "engine/vulkan/vulkan_swapchain.h"

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <GLFW/glfw3.h>

static backend_context context;

void create_sync_objets() {
  VkSemaphoreCreateInfo sem_create_info{};
  sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_create_info{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VK_CHECK(vkCreateSemaphore(context.device.logical_device, &sem_create_info,
                             nullptr, &context.image_available_semaphore));
  VK_CHECK(vkCreateSemaphore(context.device.logical_device, &sem_create_info,
                             nullptr, &context.render_finished_semaphore));
  VK_CHECK(vkCreateFence(context.device.logical_device, &fence_create_info,
                         nullptr, &context.in_flight_fence));

  return;
}

void renderer_backend_draw_frame() {
  vkWaitForFences(context.device.logical_device, 1, &context.in_flight_fence,
                  VK_TRUE, UINT64_MAX);
  vkResetFences(context.device.logical_device, 1, &context.in_flight_fence);
  uint32_t image_index;
  vkAcquireNextImageKHR(context.device.logical_device, context.swapchain.handle,
                        UINT64_MAX, context.image_available_semaphore,
                        VK_NULL_HANDLE, &image_index);
  vkResetCommandBuffer(context.command_buffer,
                       0);  // nothing special with command buffer for now;
  renderer_backend_draw_image(image_index);
  // time to submit commands now
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {context.image_available_semaphore};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  // Specify which command buffers we're submitting
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &context.command_buffer;

  // Specify which semaphore to signal once done
  VkSemaphore signal_semaphores = {context.render_finished_semaphore};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &signal_semaphores;

  VK_CHECK(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info,
                         context.in_flight_fence));

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &signal_semaphores;

  VkSwapchainKHR swapchains[] = {context.swapchain.handle};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_index;

  present_info.pResults = nullptr;
  vkQueuePresentKHR(context.device.present_queue, &present_info);
}

// TODO: Rename this as 'draw image' is a slight misnomer as it does do that
// however, it also primarily 'records commands' in vulkan terms
void renderer_backend_draw_image(uint32_t image_index) {
  // Start recording command buffer
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(context.command_buffer, &begin_info));
  // Begin renderpass
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = context.main_renderpass.handle;
  render_pass_info.framebuffer = context.swapchain.framebuffers[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = context.swapchain.extent;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};  // clear to black
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(context.command_buffer, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Bind pipeline
  vkCmdBindPipeline(context.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    context.pipeline.handle);

  // Create viewport and scissor since we specified dynamic earlier
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(context.swapchain.extent.width);
  viewport.height = static_cast<float>(context.swapchain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(context.command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = context.swapchain.extent;
  vkCmdSetScissor(context.command_buffer, 0, 1, &scissor);

  // WOOOOOOO
  // TODO: make this like, way more configurable
  vkCmdDraw(context.command_buffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(context.command_buffer);

  VK_CHECK(vkEndCommandBuffer(context.command_buffer));
}

// --------- SETUP / TEARDOWN FUNCTIONS ---------------
void create_command_pool() {
  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = context.device.graphics_queue_index;
  VK_CHECK(vkCreateCommandPool(context.device.logical_device, &pool_create_info,
                               nullptr, &context.command_pool));
  return;
}
void create_command_buffer() {
  VkCommandBufferAllocateInfo cmdbuffer_alloc_info{};
  cmdbuffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdbuffer_alloc_info.commandPool = context.command_pool;
  cmdbuffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdbuffer_alloc_info.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(context.device.logical_device,
                                    &cmdbuffer_alloc_info,
                                    &context.command_buffer));
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

bool check_validation_layer_support();

void generate_framebuffers(backend_context *context) {
  context->swapchain.framebuffers.resize(context->swapchain.views.size());
  for (size_t i = 0; i < context->swapchain.views.size(); i++) {
    VkImageView attachments[] = {context->swapchain.views[i]};

    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = context->main_renderpass.handle;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = attachments;
    framebuffer_create_info.width = context->swapchain.extent.width;
    framebuffer_create_info.height = context->swapchain.extent.height;
    framebuffer_create_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(context->device.logical_device,
                                 &framebuffer_create_info, nullptr,
                                 &context->swapchain.framebuffers[i]));
  }
  OE_LOG(LOG_LEVEL_INFO, "Framebuffers generated");
  return;
}

bool renderer_backend_initialize(platform_state *plat_state) {
  // Initialize Vulkan Instance

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_2;
  appInfo.pApplicationName = "Parallax";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Orion Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;

  // Extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensions.data());
  // Create a vector to hold the extension names
  std::vector<const char *> extensionNames;
  for (const auto &extension : extensions) {
    extensionNames.push_back(
        extension.extensionName);  // Add the extension name to the vector
  }
  OE_LOG(LOG_LEVEL_DEBUG, "available extensions:");

  for (const auto &extension : extensionNames) {
    OE_LOG(LOG_LEVEL_DEBUG, "\t%s", extension);
  }

  // This is initialized here, if in debug mode they will be overwritten
  createInfo.enabledLayerCount = 0;

  createInfo.pNext = nullptr;

#ifdef NDEBUG

  extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  for (size_t i = 0; i < glfwExtensionCount; i++) {
    extensionNames.push_back(glfwExtensions[i]);
  }
  extensionCount += glfwExtensionCount;

  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensionNames.data();

  if (!check_validation_layer_support()) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to get validation layers!");
    return false;
  }

  if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to create vulkan instance!");
    return false;
  }
  // Debugger
#ifndef NDEBUG

  uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debug_create_info.messageSeverity = log_severity;
  debug_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug_create_info.pfnUserCallback = vk_debug_callback;

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          context.instance, "vkCreateDebugUtilsMessengerEXT");
  OE_ASSERT_MSG(func, "Failed to create debug messenger!");
  VK_CHECK(func(context.instance, &debug_create_info, nullptr,
                &context.debug_messenger));

  OE_LOG(LOG_LEVEL_DEBUG, "Vulkan debugger created.");
  createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
#endif

  // Save a ref to the window from the plat platform state
  context.window = plat_state->window;
  // Now make the surface - we're using GLFW so just....use it
  VK_CHECK(glfwCreateWindowSurface(context.instance, context.window, nullptr,
                                   &context.surface));
  OE_LOG(LOG_LEVEL_DEBUG, "Vulkan surface created!");
  // Device, both physical and logical, queues included
  if (!vulkan_device_create(&context)) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to find physical device!");
    return false;
  }

  // Create swapchain and associated imageviews
  // TODO: Consider moving create_image_views into the swapchain create, not
  // really relevant to backend as a separate step
  vulkan_swapchain_create(&context);
  vulkan_swapchain_create_image_views(&context);

  // Main renderpass
  vulkan_renderpass_create(&context, &context.main_renderpass);

  OE_LOG(LOG_LEVEL_INFO, "Main renderpass created");

  // Creates the pipeline as well.
  // TODO: support multiple...shaders/pipelines?
  vulkan_shader_create(&context, &context.main_renderpass, "default.vert.glsl",
                       "default.frag.glsl");

  generate_framebuffers(&context);

  // Create command buffers
  create_command_pool();
  create_command_buffer();

  create_sync_objets();

  return true;
}

void renderer_backend_shutdown() {
  // Opposite order of creation

  vkDestroyFence(context.device.logical_device, context.in_flight_fence,
                 nullptr);
  vkDestroySemaphore(context.device.logical_device,
                     context.image_available_semaphore, nullptr);

  vkDestroySemaphore(context.device.logical_device,
                     context.render_finished_semaphore, nullptr);

  vkDestroyCommandPool(context.device.logical_device, context.command_pool,
                       nullptr);
  for (auto framebuffer : context.swapchain.framebuffers) {
    vkDestroyFramebuffer(context.device.logical_device, framebuffer, nullptr);
  }
  vkDestroyPipelineLayout(context.device.logical_device,
                          context.pipeline.layout, nullptr);
  // Image views
  for (auto image_view : context.swapchain.views) {
    vkDestroyImageView(context.device.logical_device, image_view, nullptr);
  }
  vkDestroySwapchainKHR(context.device.logical_device, context.swapchain.handle,
                        nullptr);
  vkDestroyDevice(context.device.logical_device, nullptr);
  vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

#ifndef NDEBUG

  // Destroy debug messenger
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      context.instance, "vkDestroyDebugUtilsMessengerEXT");
  func(context.instance, context.debug_messenger, nullptr);

#endif
  vkDestroyInstance(context.instance, nullptr);
}

bool check_validation_layer_support() {
  // Validation layers.
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

#ifndef NDEBUG
  OE_LOG(LOG_LEVEL_DEBUG, "Validation layers enabled. Enumerating...");

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  OE_LOG(LOG_LEVEL_DEBUG, "Available layer count: %d", layerCount);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  OE_LOG(LOG_LEVEL_DEBUG, "Available layers:");
  for (const auto &layerProperties : availableLayers) {
    OE_LOG(LOG_LEVEL_DEBUG, "\t%s", layerProperties.layerName);
  }

  for (const char *layerName : validationLayers) {
    bool layerFound = false;
    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        OE_LOG(LOG_LEVEL_DEBUG, "%s layer found", layerName);
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  OE_LOG(LOG_LEVEL_DEBUG, "All required validation layers are present.");

#endif
  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_types,
                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                  void *user_data) {
  // TODO: Check Error level for verbosity reasons
  OE_LOG(LOG_LEVEL_DEBUG, "%s", callback_data->pMessage);
  return false;
}
