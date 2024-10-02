
#include "engine/renderer_backend.h"

#include <vulkan/vulkan_core.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>

#include "engine/logger.h"
#include "engine/platform.h"
#include "engine/renderer_types.inl"
#include "engine/vulkan/vulkan_buffer.h"
#include "engine/vulkan/vulkan_device.h"
#include "engine/vulkan/vulkan_image.h"
#include "engine/vulkan/vulkan_renderpass.h"
#include "engine/vulkan/vulkan_shader.h"
#include "engine/vulkan/vulkan_swapchain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static backend_context context;

void renderer_create_texture() {
  // TODO: Temp code
  int width, height, channels;
  void *pixels;
  platform_open_image("textures/statue.jpg", &width, &height, &channels,
                      &pixels);

  vulkan_buffer staging;
  vulkan_buffer_create(&context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       width * height * 4, &staging);

  vulkan_buffer_load_data(&context, &staging, 0, 0, width * height * 4, pixels);

  vulkan_image default_tex_image;
  vulkan_image_create(&context, width, height, &default_tex_image);
  vulkan_image_transition_layout(
      &context, &default_tex_image, VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan_image_copy_from_buffer(&context, staging, default_tex_image, height,
                                width);
  vulkan_image_transition_layout(&context, &default_tex_image,
                                 VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vulkan_buffer_destroy(&context, &staging);

  vulkan_image_create_view(&context, &default_tex_image);
}

void create_descriptor_pool() {
  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VK_CHECK(vkCreateDescriptorPool(context.device.logical_device, &pool_info,
                                  nullptr, &context.descriptor_pool));
}

void create_descriptor_set() {
  std::vector<VkDescriptorSetLayout> layouts(
      MAX_FRAMES_IN_FLIGHT, context.pipeline.descriptor_set_layout);
  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = context.descriptor_pool;
  alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  alloc_info.pSetLayouts = layouts.data();

  context.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

  VK_CHECK(vkAllocateDescriptorSets(context.device.logical_device, &alloc_info,
                                    context.descriptor_sets.data()));

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = context.uniform_buffers[i].handle;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = context.descriptor_sets[i];
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;
    descriptor_write.pImageInfo = nullptr;
    descriptor_write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(context.device.logical_device, 1, &descriptor_write,
                           0, nullptr);
  }
}

void create_buffers() {
  // TODO: Buffers shouldn't be hardcoded like this. Revist after geometry
  // system
  // Vertices
  vulkan_buffer staging;
  vulkan_buffer_create(&context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       sizeof(vertices[0]) * vertices.size(), &staging);

  vulkan_buffer_load_data(&context, &staging, 0, 0,
                          sizeof(vertices[0]) * vertices.size(),
                          vertices.data());
  vulkan_buffer_create(
      &context,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      sizeof(vertices[0]) * vertices.size(), &context.vert_buff);

  vulkan_buffer_copy(&context, &staging, &context.vert_buff,
                     sizeof(vertices[0]) * vertices.size());

  // Indices
  vulkan_buffer index_staging;
  vulkan_buffer_create(&context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       sizeof(indices[0]) * indices.size(), &index_staging);
  vulkan_buffer_create(
      &context,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(indices[0]) * indices.size(),
      &context.index_buff);

  vulkan_buffer_load_data(&context, &index_staging, 0, 0,
                          sizeof(indices[0]) * indices.size(), indices.data());

  vulkan_buffer_copy(&context, &index_staging, &context.index_buff,
                     sizeof(indices[0]) * indices.size());

  // Uniforms
  context.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
  context.uniform_buffer_memory.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i <= MAX_FRAMES_IN_FLIGHT; i++) {
    vulkan_buffer_create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         sizeof(UniformBufferObject),
                         &context.uniform_buffers[i]);
    // Map the memory here since we will be updating this *constantly*
    vkMapMemory(
        context.device.logical_device, context.uniform_buffers[i].memory, 0,
        sizeof(UniformBufferObject), 0, &context.uniform_buffer_memory[i]);
  }
}
void create_sync_objects() {
  context.image_available_semaphore.resize(MAX_FRAMES_IN_FLIGHT);
  context.render_finished_semaphore.resize(MAX_FRAMES_IN_FLIGHT);
  context.in_flight_fence.resize(MAX_FRAMES_IN_FLIGHT);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkSemaphoreCreateInfo sem_create_info{};
    sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateSemaphore(context.device.logical_device, &sem_create_info,
                               nullptr, &context.image_available_semaphore[i]));
    VK_CHECK(vkCreateSemaphore(context.device.logical_device, &sem_create_info,
                               nullptr, &context.render_finished_semaphore[i]));
    VK_CHECK(vkCreateFence(context.device.logical_device, &fence_create_info,
                           nullptr, &context.in_flight_fence[i]));
  }
  return;
}

void update_ubo(uint32_t frame_index) {
  static auto start_time = std::chrono::high_resolution_clock::now();

  auto current_time = std::chrono::high_resolution_clock::now();

  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   current_time - start_time)
                   .count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(
      glm::radians(45.0f),
      context.swapchain.extent.width / (float)context.swapchain.extent.height,
      0.1f, 10.0f);
  ubo.proj[1][1] *= -1;  // we're not in openGL
  memcpy(context.uniform_buffer_memory[frame_index], &ubo, sizeof(ubo));
}
void renderer_backend_draw_frame() {
  vkWaitForFences(context.device.logical_device, 1,
                  &context.in_flight_fence[context.current_frame], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(context.device.logical_device, 1,
                &context.in_flight_fence[context.current_frame]);
  uint32_t image_index;
  vkAcquireNextImageKHR(
      context.device.logical_device, context.swapchain.handle, UINT64_MAX,
      context.image_available_semaphore[context.current_frame], VK_NULL_HANDLE,
      &image_index);
  vkResetCommandBuffer(context.command_buffer[context.current_frame],
                       0);  // nothing special with command buffer for now;

  update_ubo(context.current_frame);
  renderer_backend_draw_image(image_index);
  // time to submit commands now
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
      context.image_available_semaphore[context.current_frame]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  // Specify which command buffers we're submitting
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &context.command_buffer[context.current_frame];

  // Specify which semaphore to signal once done
  VkSemaphore signal_semaphores = {
      context.render_finished_semaphore[context.current_frame]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &signal_semaphores;

  VK_CHECK(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info,
                         context.in_flight_fence[context.current_frame]));

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

  // ONCE DONE WITH FRAME, INCREMENT HERE
  context.current_frame = (context.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// TODO: Rename this as 'draw image' is a slight misnomer as it does do that
// however, it also primarily 'records commands' in vulkan terms
void renderer_backend_draw_image(uint32_t image_index) {
  // Start recording command buffer
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(context.command_buffer[context.current_frame],
                                &begin_info));
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

  vkCmdBeginRenderPass(context.command_buffer[context.current_frame],
                       &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // Bind pipeline
  vkCmdBindPipeline(context.command_buffer[context.current_frame],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline.handle);

  VkBuffer vertex_buffers[] = {context.vert_buff.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(context.command_buffer[context.current_frame], 0, 1,
                         vertex_buffers, offsets);

  vkCmdBindIndexBuffer(context.command_buffer[context.current_frame],
                       context.index_buff.handle, 0, VK_INDEX_TYPE_UINT16);

  // Create viewport and scissor since we specified dynamic earlier
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(context.swapchain.extent.width);
  viewport.height = static_cast<float>(context.swapchain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(context.command_buffer[context.current_frame], 0, 1,
                   &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = context.swapchain.extent;
  vkCmdSetScissor(context.command_buffer[context.current_frame], 0, 1,
                  &scissor);

  vkCmdBindDescriptorSets(
      context.command_buffer[context.current_frame],
      VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline.layout, 0, 1,
      &context.descriptor_sets[context.current_frame], 0, nullptr);

  // WOOOOOOO
  // TODO: make this like, way more configurable
  vkCmdDrawIndexed(context.command_buffer[context.current_frame],
                   static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
  vkCmdEndRenderPass(context.command_buffer[context.current_frame]);

  VK_CHECK(vkEndCommandBuffer(context.command_buffer[context.current_frame]));
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
  context.command_buffer.resize(MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo cmdbuffer_alloc_info{};
  cmdbuffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdbuffer_alloc_info.commandPool = context.command_pool;
  cmdbuffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdbuffer_alloc_info.commandBufferCount = context.command_buffer.size();

  VK_CHECK(vkAllocateCommandBuffers(context.device.logical_device,
                                    &cmdbuffer_alloc_info,
                                    context.command_buffer.data()));
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

  context.current_frame = 0;
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

#ifndef NDEBUG

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
  std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  createInfo.enabledLayerCount = validationLayers.size();

  createInfo.ppEnabledLayerNames = validationLayers.data();

  if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to create vulkan instance!");
    return false;
  }
  // Debugger
#ifndef NDEBUG

  uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

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
  vulkan_shader_create(&context, &context.main_renderpass, "default.vert.spv",
                       "default.frag.spv");

  generate_framebuffers(&context);

  // Create command buffers
  create_command_pool();
  create_command_buffer();

  create_sync_objects();

  create_buffers();

  create_descriptor_pool();
  create_descriptor_set();

  // renderer_create_texture();

  create_descriptor_pool();
  create_descriptor_set();
  return true;
}

void renderer_backend_shutdown() {
  vkDeviceWaitIdle(context.device.logical_device);

  vulkan_swapchain_destroy(&context);
  // TODO: Fix this
  // vkDestroyImageView(context.device.logical_device, default_tex_image,
  // nullptr);

  // vkDestroyImage(context.device.logical_device, textureImage, nullptr);
  // vkFreeMemory(device, textureImageMemory, nullptr);

  // Opposite order of creation
  vkDeviceWaitIdle(context.device.logical_device);

  for (size_t i = 0; i < context.uniform_buffers.size(); i++) {
    vulkan_buffer_destroy(&context, &context.uniform_buffers[i]);
  }
  vulkan_buffer_destroy(&context, &context.vert_buff);
  vulkan_buffer_destroy(&context, &context.index_buff);

  vulkan_swapchain_destroy(&context);

  vkDestroyDescriptorPool(context.device.logical_device,
                          context.descriptor_pool, nullptr);

  vkDestroyDescriptorSetLayout(context.device.logical_device,
                               context.pipeline.descriptor_set_layout, nullptr);

  vkDestroyPipeline(context.device.logical_device, context.pipeline.handle,
                    nullptr);
  vkDestroyPipelineLayout(context.device.logical_device,
                          context.pipeline.layout, nullptr);
  vkDestroyRenderPass(context.device.logical_device,
                      context.main_renderpass.handle, nullptr);
  // cleanup any buffers

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyFence(context.device.logical_device, context.in_flight_fence[i],
                   nullptr);
    vkDestroySemaphore(context.device.logical_device,
                       context.image_available_semaphore[i], nullptr);

    vkDestroySemaphore(context.device.logical_device,
                       context.render_finished_semaphore[i], nullptr);
  }

  vkFreeCommandBuffers(context.device.logical_device, context.command_pool,
                       context.command_buffer.size(),
                       context.command_buffer.data());

  vkDestroyCommandPool(context.device.logical_device, context.command_pool,
                       nullptr);

  vkDestroyDevice(context.device.logical_device, nullptr);

#ifndef NDEBUG

  // Destroy debug messenger
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      context.instance, "vkDestroyDebugUtilsMessengerEXT");
  func(context.instance, context.debug_messenger, nullptr);

#endif

  vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
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
