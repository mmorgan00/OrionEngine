#ifndef RENDERER_TYPES
#define RENDERER_TYPES

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "engine/asserts.h"

typedef struct vulkan_swapchain_support_info {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t format_count;
  std::vector<VkSurfaceFormatKHR> formats;
  uint32_t present_mode_count;
  VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;
  vulkan_swapchain_support_info swapchain_support;
  int graphics_queue_index;
  int present_queue_index;
  int transfer_queue_index;

  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;

  VkCommandPool graphics_command_pool;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_renderpass {
  VkRenderPass handle;
} vulkan_renderpass;

typedef struct vulkan_swapchain {
  VkFormat image_format;
  short max_frames_in_flight;
  VkSwapchainKHR handle;
  VkExtent2D extent;
  uint32_t image_count;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;  // images not accessed directly in Vulkan
  std::vector<VkFramebuffer> framebuffers;
} vulkan_swapchain;

typedef struct vulkan_pipeline {
  VkPipeline handle;
  VkPipelineLayout layout;
} vulkan_pipeline;

typedef struct vulkan_shader_stage {
  VkShaderModuleCreateInfo create_info;
  VkShaderModule handle;
  VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

#define OBJECT_SHADER_STAGE_COUNT 2  // vertex, fragment for now

typedef struct vulkan_object_shader {
  // vertex, fragment
  vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];

} vulkan_object_shader;

typedef struct backend_context {
  VkInstance instance;
  vulkan_device device;
  GLFWwindow* window;
  VkSurfaceKHR surface;
  vulkan_pipeline pipeline;
  vulkan_renderpass main_renderpass;
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
  vulkan_swapchain swapchain;
  VkSemaphore image_available_semaphore;
  VkSemaphore render_finished_semaphore;
  VkFence in_flight_fence;
} backend_context;

#define VK_CHECK(expr)               \
  do {                               \
    VkResult result = (expr);        \
    OE_ASSERT(result == VK_SUCCESS); \
  } while (0)

#endif
