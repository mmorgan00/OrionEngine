
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
  vulkan_buffer_create(&context, vk::BufferUsageFlagBits::eTransferSrc,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
                       width * height * 4, &staging);

  vulkan_buffer_load_data(&context, &staging, 0, 0, width * height * 4, pixels);

  vulkan_image_create(&context, height, width, &context.default_texture.image);
  vulkan_image_transition_layout(
      &context, &context.default_texture.image, vk::Format::eR8G8B8A8Srgb,
      vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
  vulkan_image_copy_from_buffer(&context, staging,
                                context.default_texture.image, height, width);
  vulkan_image_transition_layout(&context, &context.default_texture.image,
                                 vk::Format::eB8G8R8A8Srgb,
                                 vk::ImageLayout::eTransferDstOptimal,
                                 vk::ImageLayout::eShaderReadOnlyOptimal);

  vulkan_buffer_destroy(&context, &staging);

  vulkan_image_create_view(&context, vk::Format::eR8G8B8A8Srgb,
                           &context.default_texture.image.handle,
                           &context.default_texture.image.view);

  vulkan_image_create_sampler(&context, &context.default_texture.image,
                              &context.default_texture.sampler);
}

void create_descriptor_pool() {
  vk::DescriptorPoolSize buffer_ps{
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
  };
  vk::DescriptorPoolSize sampler_ps{
      .type = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
  };
  std::array<vk::DescriptorPoolSize, 2> pool_size = {buffer_ps, sampler_ps};

  vk::DescriptorPoolCreateInfo pool_info{
      .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
      .pPoolSizes = pool_size.data(),
  };

  context.descriptor_pool =
      context.device.logical_device.createDescriptorPool(pool_info);
}

void create_descriptor_set() {
  std::vector<vk::DescriptorSetLayout> layouts(
      MAX_FRAMES_IN_FLIGHT, context.pipeline.descriptor_set_layout);

  vk::DescriptorSetAllocateInfo alloc_info{
      .descriptorPool = context.descriptor_pool,
      .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .pSetLayouts = layouts.data()

  };
  context.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
  context.descriptor_sets =
      context.device.logical_device.allocateDescriptorSets(alloc_info);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo buffer_info{
        .buffer = context.uniform_buffers[i].handle,
        .offset = 0,
        .range = sizeof(UniformBufferObject)};

    vk::DescriptorImageInfo image_info{
        .sampler = context.default_texture.sampler,
        .imageView = context.default_texture.image.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    vk::WriteDescriptorSet uniform_buffer_descriptor_write{
        .dstSet = context.descriptor_sets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr};
    vk::WriteDescriptorSet texture_descriptor_write{
        .dstSet = context.descriptor_sets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info};
    std::array<vk::WriteDescriptorSet, 2> descriptor_write{
        uniform_buffer_descriptor_write, texture_descriptor_write};

    context.device.logical_device.updateDescriptorSets(descriptor_write, 0);
  }
}

void create_buffers() {
  // TODO: Buffers shouldn't be hardcoded like this. Revist after geometry
  // system
  // Vertices
  vulkan_buffer staging;
  vulkan_buffer_create(&context, vk::BufferUsageFlagBits::eTransferSrc,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
                       sizeof(vertices[0]) * vertices.size(), &staging);

  vulkan_buffer_load_data(&context, &staging, 0, 0,
                          sizeof(vertices[0]) * vertices.size(),
                          vertices.data());
  vulkan_buffer_create(&context,
                       vk::BufferUsageFlagBits::eTransferDst |
                           vk::BufferUsageFlagBits::eVertexBuffer,
                       vk::MemoryPropertyFlagBits::eDeviceLocal,
                       sizeof(vertices[0]) * vertices.size(),
                       &context.vert_buff);

  vulkan_buffer_copy(&context, &staging, &context.vert_buff,
                     sizeof(vertices[0]) * vertices.size());

  // Indices
  vulkan_buffer index_staging;
  vulkan_buffer_create(&context, vk::BufferUsageFlagBits::eTransferSrc,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent,
                       sizeof(indices[0]) * indices.size(), &index_staging);
  vulkan_buffer_create(&context,
                       vk::BufferUsageFlagBits::eTransferDst |
                           vk::BufferUsageFlagBits::eIndexBuffer,
                       vk::MemoryPropertyFlagBits::eDeviceLocal,
                       sizeof(indices[0]) * indices.size(),
                       &context.index_buff);

  vulkan_buffer_load_data(&context, &index_staging, 0, 0,
                          sizeof(indices[0]) * indices.size(), indices.data());

  vulkan_buffer_copy(&context, &index_staging, &context.index_buff,
                     sizeof(indices[0]) * indices.size());

  // Uniforms
  context.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
  context.uniform_buffer_memory.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i <= MAX_FRAMES_IN_FLIGHT; i++) {
    vulkan_buffer_create(&context, vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent,
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
    vk::SemaphoreCreateInfo sem_create_info{};

    vk::FenceCreateInfo fence_create_info{
        .flags = vk::FenceCreateFlagBits::eSignaled};

    context.image_available_semaphore[i] =
        context.device.logical_device.createSemaphore(sem_create_info);
    context.render_finished_semaphore[i] =
        context.device.logical_device.createSemaphore(sem_create_info);
    context.in_flight_fence[i] =
        context.device.logical_device.createFence(fence_create_info);
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
  vk::Device device = context.device.logical_device;
  VK_CHECK(device.waitForFences(1,
                                &context.in_flight_fence[context.current_frame],
                                vk::True, UINT64_MAX));
  VK_CHECK(
      device.resetFences(1, &context.in_flight_fence[context.current_frame]));

  uint32_t image_index;
  vk::ResultValue<uint32_t> result = device.acquireNextImageKHR(
      context.swapchain.handle, UINT64_MAX,
      context.image_available_semaphore[context.current_frame], VK_NULL_HANDLE);
  if (result.result != vk::Result::eSuccess) {
    OE_LOG(LOG_LEVEL_ERROR, "Failed to acquire next image!");
    return;
  }
  context.command_buffer[context.current_frame].reset();

  update_ubo(context.current_frame);
  renderer_backend_draw_image(image_index);
  // time to submit commands now

  vk::Semaphore wait_semaphores[] = {
      context.image_available_semaphore[context.current_frame]};
  vk::PipelineStageFlags wait_stages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::Semaphore signal_semaphores = {
      context.render_finished_semaphore[context.current_frame]};

  vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &context.command_buffer[context.current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &signal_semaphores,
  };

  context.device.graphics_queue.submit(
      submit_info, context.in_flight_fence[context.current_frame]);

  vk::SwapchainKHR swapchains[] = {context.swapchain.handle};
  vk::PresentInfoKHR present_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &signal_semaphores,
      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &image_index,
      .pResults = nullptr,
  };

  VK_CHECK(context.device.present_queue.presentKHR(present_info));

  // ONCE DONE WITH FRAME, INCREMENT HERE
  context.current_frame = (context.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// TODO: Rename this as 'draw image' is a slight misnomer as it does do that
// however, it also primarily 'records commands' in vulkan terms
void renderer_backend_draw_image(uint32_t image_index) {
  auto cmd_buff =
      context.command_buffer[context.current_frame];  // Shorthand because we
                                                      // use this A TON
  // Start recording command buffer
  vk::CommandBufferBeginInfo begin_info{};
  cmd_buff.begin(begin_info);
  // Begin renderpass

  std::array<float, 4> color_constants_array = {0.0f, 0.0f, 0.0f,
                                                1.0f};  // Black color

  vk::ClearColorValue clear_color_value{.float32 = color_constants_array};

  vk::ClearValue clear_color{.color = clear_color_value};

  vk::RenderPassBeginInfo render_pass_info{
      .renderPass = context.main_renderpass.handle,
      .framebuffer = context.swapchain.framebuffers[image_index],
      .renderArea = {.offset = {.x = 0, .y = 0},
                     .extent = context.swapchain.extent},
      .clearValueCount = 1,
      .pClearValues = &clear_color  // Use address-of operator for pointer
  };
  cmd_buff.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
  // Bind pipeline
  cmd_buff.bindPipeline(vk::PipelineBindPoint::eGraphics,
                        context.pipeline.handle);

  vk::Buffer vertex_buffers[] = {context.vert_buff.handle};
  vk::DeviceSize offsets[] = {0};
  // context.command_buffer[context..current_frame]
  cmd_buff.bindVertexBuffers(0, 1, vertex_buffers, offsets);

  cmd_buff.bindIndexBuffer(context.index_buff.handle, 0,
                           vk::IndexType::eUint16);

  // Create viewport and scissor since we specified dynamic earlier
  vk::Viewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(context.swapchain.extent.width),
      .height = static_cast<float>(context.swapchain.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 0.0f,
  };
  cmd_buff.setViewport(0, 1, &viewport);

  vk::Rect2D scissor{.offset = {.x = 0, .y = 0},
                     .extent = context.swapchain.extent};
  cmd_buff.setScissor(0, 1, &scissor);

  cmd_buff.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, context.pipeline.layout, 0, 1,
      &context.descriptor_sets[context.current_frame], 0, nullptr);

  // WOOOOOOO
  // TODO: make this like, way more configurable
  cmd_buff.drawIndexed(indices.size(), 1, 0, 0, 0);

  cmd_buff.endRenderPass();

  // TODO: Check result
  cmd_buff.end();
}

// --------- SETUP / TEARDOWN FUNCTIONS ---------------
void create_command_pool() {
  vk::CommandPoolCreateInfo pool_create_info{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex =
          static_cast<uint32_t>(context.device.graphics_queue_index),
  };
  context.command_pool = context.device.logical_device.createCommandPool(
      pool_create_info, nullptr);
  return;
}
void create_command_buffer() {
  context.command_buffer.resize(MAX_FRAMES_IN_FLIGHT);
  vk::CommandBufferAllocateInfo cmdbuffer_alloc_info{
      .commandPool = context.command_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount =
          static_cast<uint32_t>(context.command_buffer.size())};

  context.command_buffer = context.device.logical_device.allocateCommandBuffers(
      cmdbuffer_alloc_info);
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

bool check_validation_layer_support();

void generate_framebuffers(backend_context *context) {
  context->swapchain.framebuffers.resize(context->swapchain.images.size());
  for (size_t i = 0; i < context->swapchain.images.size(); i++) {
    std::array<vk::ImageView, 1> attachments = {context->swapchain.views[i]};

    vk::FramebufferCreateInfo framebuffer_ci{
        .renderPass = context->main_renderpass.handle,
        .attachmentCount = 1,
        .pAttachments = attachments.data(),
        .width = context->swapchain.extent.width,
        .height = context->swapchain.extent.height,
        .layers = 1,
    };
    context->swapchain.framebuffers[i] =
        context->device.logical_device.createFramebuffer(framebuffer_ci);
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

  context.instance = vk::createInstanceUnique(ci);

  if (!context.instance) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to create vulkan instance!");
    return false;
  }

// Debugger
#ifndef NDEBUG
  pfnVkCreateDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
          context.instance.get().getProcAddr("vkCreateDebugUtilsMessengerEXT"));
  if (!pfnVkCreateDebugUtilsMessengerEXT) {
    OE_LOG(LOG_LEVEL_ERROR,
           "GetInstanceProcAddr: Unable to find "
           "pfnVkCreateDebugUtilsMessengerEXT function.");
    return false;
  }

  pfnVkDestroyDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          context.instance.get().getProcAddr(
              "vkDestroyDebugUtilsMessengerEXT"));
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
      context.instance.get().createDebugUtilsMessengerEXT(debug_ci);

  OE_ASSERT_MSG(context.debug_messenger, "Failed to create debug messenger!");

  OE_LOG(LOG_LEVEL_DEBUG, "Vulkan debugger created.");
  ci.pNext = &debug_ci;

#endif

  // Save a ref to the window from the plat platform state
  context.window = plat_state->window;
  // Now make the surface - we're using GLFW so just....use it
  VkSurfaceKHR temp_surface;
  glfwCreateWindowSurface(context.instance.get(), context.window, nullptr,
                          &temp_surface);
  context.surface = vk::SurfaceKHR(temp_surface);

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
  try {
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
                                 context.pipeline.descriptor_set_layout,
                                 nullptr);

    // vulkan_shader_destroy(&context);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyFence(context.device.logical_device, context.in_flight_fence[i],
                     nullptr);
      vkDestroySemaphore(context.device.logical_device,
                         context.image_available_semaphore[i], nullptr);

      vkDestroySemaphore(context.device.logical_device,
                         context.render_finished_semaphore[i], nullptr);
    }

    // vkFreeCommandBuffers(context.device.logical_device, context.command_pool,
    //                      context.command_buffer.size(),
    //                      context.command_buffer.data());

    vkDestroyCommandPool(context.device.logical_device, context.command_pool,
                         nullptr);

    vkDestroyRenderPass(context.device.logical_device,
                        context.main_renderpass.handle, nullptr);

    OE_LOG(LOG_LEVEL_INFO, "Destroying swapchain");
    // vulkan_swapchain_destroy(&context);
    // TODO: Fix this
    // vkDestroyImageView(context.device.logical_device, default_tex_image,
    // nullptr);

    // vkDestroyImage(context.device.logical_device, textureImage, nullptr);
    // vkFreeMemory(device, textureImageMemory, nullptr);

    // Opposite order of creation

    vkDestroyDevice(context.device.logical_device, nullptr);

    vkDestroySurfaceKHR(context.instance.get(), context.surface, nullptr);
#ifndef NDEBUG
    context.instance.get().destroyDebugUtilsMessengerEXT(
        context.debug_messenger);
#endif
  } catch (vk::SystemError &err) {
    OE_LOG(LOG_LEVEL_ERROR, "vk::SystemError: %s", err.what());
    exit(-1);
  } catch (std::exception &err) {
    OE_LOG(LOG_LEVEL_ERROR, "std::exceptoin: %s", err.what());
    exit(-1);
  } catch (...) {
    OE_LOG(LOG_LEVEL_ERROR, "unknown error");
    exit(-1);
  }
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
