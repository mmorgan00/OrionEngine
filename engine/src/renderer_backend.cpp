
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

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger) {
  return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator,
                                           pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT messenger,
    VkAllocationCallbacks const *pAllocator) {
  return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
void renderer_create_texture() {
  // TODO: Temp code
  int width, height, channels;
  void *pixels =
      platform_open_image("textures/texture.jpg", &height, &width, &channels);

  vulkan_buffer staging;
  vulkan_buffer_create(&context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       width * height * 4, &staging);

  vulkan_buffer_load_data(&context, &staging, 0, 0, width * height * 4, pixels);

  vulkan_image_create(&context, height, width, &context.default_texture.image);
  vulkan_image_transition_layout(
      &context, &context.default_texture.image, VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan_image_copy_from_buffer(&context, staging,
                                context.default_texture.image, height, width);
  vulkan_image_transition_layout(&context, &context.default_texture.image,
                                 VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vulkan_buffer_destroy(&context, &staging);

  vulkan_image_create_view(&context, VK_FORMAT_R8G8B8A8_SRGB,
                           &context.default_texture.image.handle,
                           &context.default_texture.image.view);

  VkSampler default_tex_sampler;
  vulkan_image_create_sampler(&context, &context.default_texture.image,
                              &context.default_texture.sampler);
}

void create_descriptor_pool() {
  std::array<VkDescriptorPoolSize, 2> pool_size{};
  pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());
  pool_info.pPoolSizes = pool_size.data();
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

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = context.default_texture.image.view;
    image_info.sampler = context.default_texture.sampler;

    std::array<VkWriteDescriptorSet, 2> descriptor_write{};
    descriptor_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[0].dstSet = context.descriptor_sets[i];
    descriptor_write[0].dstBinding = 0;
    descriptor_write[0].dstArrayElement = 0;
    descriptor_write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write[0].descriptorCount = 1;
    descriptor_write[0].pBufferInfo = &buffer_info;
    descriptor_write[0].pImageInfo = nullptr;
    descriptor_write[0].pTexelBufferView = nullptr;

    descriptor_write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write[1].dstSet = context.descriptor_sets[i];
    descriptor_write[1].dstBinding = 1;
    descriptor_write[1].dstArrayElement = 0;
    descriptor_write[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write[1].descriptorCount = 1;
    descriptor_write[1].pImageInfo = &image_info;

    vkUpdateDescriptorSets(context.device.logical_device,
                           descriptor_write.size(), descriptor_write.data(), 0,
                           nullptr);
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
  context->swapchain.framebuffers.resize(context->swapchain.images.size());
  for (size_t i = 0; i < context->swapchain.images.size(); i++) {
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
  vk::ApplicationInfo app_info{.pApplicationName = "Parallax",
                               .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                               .pEngineName = "Orion Engine",
                               .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                               .apiVersion = VK_API_VERSION_1_2};

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_2;
  appInfo.pApplicationName = "Parallax";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Orion Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;

  // Extensions
  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         extensions.data());
  // Create a vector to hold the extension names
  std::vector<const char *> extension_names;
  for (const auto &extension : extensions) {
    extension_names.push_back(
        extension.extensionName);  // Add the extension name to the vector
  }
  OE_LOG(LOG_LEVEL_DEBUG, "available extensions:");

  for (const auto &extension : extension_names) {
    OE_LOG(LOG_LEVEL_DEBUG, "\t%s", extension);
  }
  uint32_t enabled_layer_count = 0;
  // This is initialized here, if in debug mode they will be overwritten

#ifndef NDEBUG

  extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  for (size_t i = 0; i < glfwExtensionCount; i++) {
    extension_names.push_back(glfwExtensions[i]);
  }
  extension_count += glfwExtensionCount;

  if (!check_validation_layer_support()) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to get validation layers!");
    return false;
  }
  std::vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};

  vk::InstanceCreateInfo ci = {
      .pApplicationInfo = &app_info,
      .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
      .ppEnabledLayerNames = validation_layers.data(),
      .enabledExtensionCount = extension_count,
      .ppEnabledExtensionNames = extension_names.data()};

  context.instance = vk::createInstance(ci);

  if (!context.instance) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to create vulkan instance!");
    return false;
  }

// Debugger
#ifndef NDEBUG
  pfnVkCreateDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
          context.instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
  if (!pfnVkCreateDebugUtilsMessengerEXT) {
    OE_LOG(LOG_LEVEL_ERROR,
           "GetInstanceProcAddr: Unable to find "
           "pfnVkCreateDebugUtilsMessengerEXT function.");
    return false;
  }

  pfnVkDestroyDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          context.instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
  if (!pfnVkDestroyDebugUtilsMessengerEXT) {
    OE_LOG(LOG_LEVEL_ERROR,
           "GetInstanceProcAddr: Unable to find "
           "pfnVkDestroyDebugUtilsMessengerEXT function");
    return false;
  }
  vk::Flags<vk::DebugUtilsMessageSeverityFlagBitsEXT> log_severity =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
  vk::DebugUtilsMessengerCreateInfoEXT debug_ci = {
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
      .pfnUserCallback = vk_debug_callback};

  context.debug_messenger =
      context.instance.createDebugUtilsMessengerEXT(debug_ci);

  OE_ASSERT_MSG(context.debug_messenger, "Failed to create debug messenger!");

  OE_LOG(LOG_LEVEL_DEBUG, "Vulkan debugger created.");
  ci.pNext = &debug_ci;

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

  renderer_create_texture();

  create_descriptor_pool();
  create_descriptor_set();
  return true;
}

void renderer_backend_shutdown() {
  OE_LOG(LOG_LEVEL_INFO, "Renderer shutting down");
  vkDeviceWaitIdle(context.device.logical_device);

  vulkan_buffer_destroy(&context, &context.vert_buff);
  vulkan_buffer_destroy(&context, &context.index_buff);

  for (size_t i = 0; i < context.uniform_buffers.size(); i++) {
    vulkan_buffer_destroy(&context, &context.uniform_buffers[i]);
  }

  vkDestroyPipeline(context.device.logical_device, context.pipeline.handle,
                    nullptr);

  vkDestroyPipelineLayout(context.device.logical_device,
                          context.pipeline.layout, nullptr);

  vkDestroyDescriptorPool(context.device.logical_device,
                          context.descriptor_pool, nullptr);

  vkDestroyDescriptorSetLayout(context.device.logical_device,
                               context.pipeline.descriptor_set_layout, nullptr);

  // vulkan_shader_destroy(&context);

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

  vkDestroyRenderPass(context.device.logical_device,
                      context.main_renderpass.handle, nullptr);

  OE_LOG(LOG_LEVEL_INFO, "Destroying swapchain");
  vulkan_swapchain_destroy(&context);
  // TODO: Fix this
  // vkDestroyImageView(context.device.logical_device, default_tex_image,
  // nullptr);

  // vkDestroyImage(context.device.logical_device, textureImage, nullptr);
  // vkFreeMemory(device, textureImageMemory, nullptr);

  // Opposite order of creation

  vkDestroyDevice(context.device.logical_device, nullptr);

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
