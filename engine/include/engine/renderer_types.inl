#ifndef RENDERER_TYPES
#define RENDERER_TYPES

#include "engine/asserts.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

typedef struct vulkan_swapchain_support_info {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t format_count;
  std::vector<VkSurfaceFormatKHR> formats;
  uint32_t present_mode_count;
  VkPresentModeKHR *present_modes;
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

typedef struct backend_context {
  VkInstance instance;
  vulkan_device device;
  GLFWwindow* window;
  VkSurfaceKHR surface;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

} backend_context;


#define VK_CHECK(expr) \
    do { \
        VkResult result = (expr); \
        OE_ASSERT(result == VK_SUCCESS); \
    } while(0)

#endif
