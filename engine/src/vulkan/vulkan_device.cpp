#include "engine/vulkan/vulkan_device.h"

#include "engine/logger.h"
#include "engine/renderer_types.inl"

#include <vector>

typedef struct vulkan_physical_device_requirements {
  bool graphics;
  bool present;
  bool compute;
  bool transfer;
  std::vector<const char*> device_extension_names;
  bool sampler_anisotropy;
  bool discrete_gpu;
} vulkan_physical_device_requirements;


typedef struct vulkan_physical_device_queue_family_info {
  uint32_t graphics_family_index;
  uint32_t present_family_index;
  uint32_t compute_family_index;
  uint32_t transfer_family_index;
} vulkan_physical_device_queue_family_info;


bool select_physical_device(backend_context* context) {
  // query for GPUS
  uint32_t physical_device_count = 0;
  VK_CHECK(
      vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
  if (physical_device_count == 0) {
    OE_LOG(LOG_LEVEL_FATAL, "No devices which support Vulkan were found.");
    return false;
  }

  VkPhysicalDevice physical_devices[32];
  VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count,
                                      physical_devices));
  OE_LOG(LOG_LEVEL_INFO, "Found %i devices", physical_device_count);
  for (uint32_t i = 0; i < physical_device_count; i++) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

    vulkan_physical_device_requirements requirements = {};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    requirements.compute = true;
    requirements.sampler_anisotropy = true;
    requirements.discrete_gpu = false;
    requirements.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vulkan_physical_device_queue_family_info queue_info = {};
    bool result = true;/**physical_device_meets_requirements(
        physical_devices[i], context->surface, &properties, &features,
        &requirements, &queue_info, &context->device.swapchain_support);
**/
    if (result) {
      OE_LOG(LOG_LEVEL_INFO,"Selected device: '%s'.", properties.deviceName);
      // GPU type, etc.
      switch (properties.deviceType) {
      default:
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        OE_LOG(LOG_LEVEL_INFO,"GPU type is Unknown.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is Integrated.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        OE_LOG(LOG_LEVEL_INFO,"GPU type is Discrete.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is Virtual.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is CPU.");
        break;
      }

      OE_LOG(LOG_LEVEL_INFO, "GPU Driver version: %d.%d.%d",
            VK_VERSION_MAJOR(properties.driverVersion),
            VK_VERSION_MINOR(properties.driverVersion),
            VK_VERSION_PATCH(properties.driverVersion));

      // Vulkan API version.
      OE_LOG(LOG_LEVEL_INFO, "Vulkan API version: %d.%d.%d",
            VK_VERSION_MAJOR(properties.apiVersion),
            VK_VERSION_MINOR(properties.apiVersion),
            VK_VERSION_PATCH(properties.apiVersion));

      // Memory information
      for (uint32_t j = 0; j < memory.memoryHeapCount; ++j) {
        float memory_size_gib =
            (((float)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
        if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
          OE_LOG(LOG_LEVEL_INFO, "Local GPU memory: %.2f GiB", memory_size_gib);
        } else {
          OE_LOG(LOG_LEVEL_INFO, "Shared System memory: %.2f GiB", memory_size_gib);
        }
      }

      context->device.physical_device = physical_devices[i];
      context->device.graphics_queue_index = queue_info.graphics_family_index;
      context->device.present_queue_index = queue_info.present_family_index;
      context->device.transfer_queue_index = queue_info.transfer_family_index;
      // NOTE: set compute index here if needed.

      // Keep a copy of properties, features and memory info for later use.
      context->device.properties = properties;
      context->device.features = features;
      context->device.memory = memory;
      break;
    }
  }
  // Ensure a device was selected
  if (!context->device.physical_device) {
    OE_LOG(LOG_LEVEL_ERROR, "No physical devices were found which meet the requirements.");
    return false;
  }

  OE_LOG(LOG_LEVEL_INFO, "Physical device selected.");
  return true;
}

bool vulkan_device_create(backend_context* context) {

  // Select physical device 
  select_physical_device(context);

  return true;
}
