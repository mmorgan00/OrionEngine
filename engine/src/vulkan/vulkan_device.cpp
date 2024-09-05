#include "engine/vulkan/vulkan_device.h"

#include "engine/logger.h"
#include "engine/renderer_types.inl"

#include <vulkan/vulkan.h>

#include <vector>

typedef struct vulkan_physical_device_requirements {
  bool graphics;
  bool present;
  bool compute;
  bool transfer;
  std::vector<const char *> device_extension_names;
  bool sampler_anisotropy;
  bool discrete_gpu;
} vulkan_physical_device_requirements;

/**
 * @brief Helper struct for holding device que indices
 **/
typedef struct vulkan_physical_device_queue_family_info {
  uint32_t graphics_family_index;
  uint32_t present_family_index;
  uint32_t compute_family_index;
  uint32_t transfer_family_index;
} vulkan_physical_device_queue_family_info;

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    vulkan_swapchain_support_info *out_support_info) {

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &out_support_info->capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       nullptr);

  if (format_count != 0) {
    out_support_info->formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                         &format_count,
                                         out_support_info->formats.data());
  }

  return;
}

/**
 * @brief Helper function for checking if a given physical device meets
 * requirements
 */
bool physical_device_meets_requirements(
    VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const vulkan_physical_device_requirements *requirements,
    vulkan_physical_device_queue_family_info *out_queue_info,
    vulkan_swapchain_support_info *out_swapchain_support) {

  // Evaluate device properties to see if it meets the needs of our app
  out_queue_info->graphics_family_index = -1;
  out_queue_info->present_family_index = -1;
  out_queue_info->compute_family_index = -1;
  out_queue_info->transfer_family_index = -1;

  if (requirements->discrete_gpu) {
    if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      OE_LOG(LOG_LEVEL_INFO,
             "Device is not a discrete GPU, and one is required. Skipping.");
      return false;
    }
  }

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
  VkQueueFamilyProperties queue_families[32];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  // Look at each queue and see what queues it supports
  OE_LOG(LOG_LEVEL_INFO, "Graphics | Present | Compute | Transfer | Name");
  short min_transfer_score = 255;
  for (uint32_t i = 0; i < queue_family_count; ++i) {
    short current_transfer_score = 0;

    // Graphics queue?
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      out_queue_info->graphics_family_index = i;
      ++current_transfer_score;
    }

    // Compute queue?
    if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      out_queue_info->compute_family_index = i;
      ++current_transfer_score;
    }

    // Transfer queue?
    if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      // Take the index if it is the current lowest. This increases the
      // liklihood that it is a dedicated transfer queue.
      if (current_transfer_score <= min_transfer_score) {
        min_transfer_score = current_transfer_score;
        out_queue_info->transfer_family_index = i;
      }
    }

    // Dedicated present queue?
    VkBool32 supports_present = VK_FALSE;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                                  &supports_present));

    if (supports_present) {
      out_queue_info->present_family_index = i;
    }
  }

  // Print out the device info
  OE_LOG(LOG_LEVEL_INFO, "       %d |       %d |       %d |        %d | %s",
         out_queue_info->graphics_family_index != -1,
         out_queue_info->present_family_index != -1,
         out_queue_info->compute_family_index != -1,
         out_queue_info->transfer_family_index != -1, properties->deviceName);

  if ((!requirements->graphics ||
       (requirements->graphics &&
        out_queue_info->graphics_family_index != -1)) &&
      (!requirements->present ||
       (requirements->present && out_queue_info->present_family_index != -1)) &&
      (!requirements->compute ||
       (requirements->compute && out_queue_info->compute_family_index != -1)) &&
      (!requirements->transfer ||
       (requirements->transfer &&
        out_queue_info->transfer_family_index != -1))) {
    OE_LOG(LOG_LEVEL_INFO, "%s Device meets queue requirements.",
           properties->deviceName);
    OE_LOG(LOG_LEVEL_TRACE, "Graphics Family Index: %i",
           out_queue_info->graphics_family_index);
    OE_LOG(LOG_LEVEL_TRACE, "Present Family Index:  %i",
           out_queue_info->present_family_index);
    OE_LOG(LOG_LEVEL_TRACE, "Transfer Family Index: %i",
           out_queue_info->transfer_family_index);
    OE_LOG(LOG_LEVEL_TRACE, "Compute Family Index:  %i",
           out_queue_info->compute_family_index);
  }
  // Get swapchain features
  vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);

  // Device extensions.
  if (requirements->device_extension_names.data()) {
    OE_LOG(LOG_LEVEL_DEBUG, "Checking extensions");
    uint32_t available_extension_count = 0;
    std::vector<VkExtensionProperties> available_extensions;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        device, 0, &available_extension_count, 0));
    if (available_extension_count != 0) {

      available_extensions =
          std::vector<VkExtensionProperties>(available_extension_count);

      VK_CHECK(vkEnumerateDeviceExtensionProperties(
          device, 0, &available_extension_count, available_extensions.data()));

      uint32_t required_extension_count =
          requirements->device_extension_names.size();
      for (uint32_t i = 0; i < required_extension_count; ++i) {
        bool found = false;
        for (uint32_t j = 0; j < available_extension_count; ++j) {
          if (requirements->device_extension_names[i] ==
              available_extensions[j].extensionName) {
            found = true;
            break;
          }
        }

        if (!found) {
          OE_LOG(LOG_LEVEL_INFO,
                 "Required extension not found: '%s', skipping device.",
                 requirements->device_extension_names[i]);
          return false;
        }
      }
    }
    OE_LOG(LOG_LEVEL_DEBUG, "All extensions found on %s",
           properties->deviceName);

    // Sampler anisotropy
    if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
      OE_LOG(LOG_LEVEL_INFO,
             "Device does not support samplerAnisotropy, skipping.");
      return false;
    }

    // Device meets all requirements.
    OE_LOG(LOG_LEVEL_DEBUG, "Found device!");
    return true;
  } else {
    OE_LOG(LOG_LEVEL_DEBUG, "No extensions provided");
    return true;
  }

  return false;
}

bool select_physical_device(backend_context *context) {
  // query for GPUS that support vulkan
  uint32_t physical_device_count = 0;
  VK_CHECK(
      vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
  if (physical_device_count == 0) {
    OE_LOG(LOG_LEVEL_FATAL, "No devices which support Vulkan were found.");
    return false;
  }

  // Check for suitability
  VkPhysicalDevice physical_devices[32];
  VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count,
                                      physical_devices));

  OE_LOG(LOG_LEVEL_INFO, "Found %i devices", physical_device_count);
  // Check each device
  for (uint32_t i = 0; i < physical_device_count; i++) {
    // Start with properties, IE is it a discrete GPU (big one)
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

    // Features, such as geometry shader support etc
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

    // Memory support. Will be nice to have if we do big time graphics
    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

    // Our little handy thing.
    // TODO: Make this come from a config file maybe?
    vulkan_physical_device_requirements requirements = {};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    requirements.compute = true;
    requirements.sampler_anisotropy = true;
    requirements.discrete_gpu = false;

    // We now have the requirements we want, check if between the device
    // properties and features all are supported
    vulkan_physical_device_queue_family_info queue_info = {};
    bool result = physical_device_meets_requirements(
        physical_devices[i], context->surface, &properties, &features,
        &requirements, &queue_info, &context->device.swapchain_support);

    if (result) {
      OE_LOG(LOG_LEVEL_INFO, "Selected device: '%s'.", properties.deviceName);
#ifndef NDEBUG // We don't need to print all the details in release
      // GPU type, etc.
      switch (properties.deviceType) {
      default:
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is Unknown.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is Integrated.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        OE_LOG(LOG_LEVEL_INFO, "GPU type is Discrete.");
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
          OE_LOG(LOG_LEVEL_INFO, "Shared System memory: %.2f GiB",
                 memory_size_gib);
        }
      }
#endif
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
    OE_LOG(LOG_LEVEL_ERROR,
           "No physical devices were found which meet the requirements.");
    return false;
  }

  OE_LOG(LOG_LEVEL_INFO, "Physical device selected.");
  return true;
}

bool vulkan_device_create(backend_context *context) {

  // Select physical device
  if (!select_physical_device(context)) {
    OE_LOG(LOG_LEVEL_FATAL, "Failed to select physical device");
    return false;
  }

  OE_LOG(LOG_LEVEL_INFO, "Creating logical device...");
  // Check for shared indices
  bool present_shares_graphics_queue = context->device.graphics_queue_index ==
                                       context->device.present_queue_index;
  bool transfer_shares_graphics_queue = context->device.graphics_queue_index ==
                                        context->device.transfer_queue_index;

  uint32_t index_count = 1;
  if (!present_shares_graphics_queue) {
    index_count++;
  }
  if (!transfer_shares_graphics_queue) {
    index_count++;
  }

  uint32_t indices[32];
  short index = 0;
  indices[index++] = context->device.graphics_queue_index;
  if (!present_shares_graphics_queue) {
    indices[index++] = context->device.present_queue_index;
  }
  if (!transfer_shares_graphics_queue) {
    indices[index++] = context->device.transfer_queue_index;
  }

  VkDeviceQueueCreateInfo queue_create_infos[32];
  for (uint32_t i = 0; i < index_count; ++i) {
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].queueFamilyIndex = indices[i];
    queue_create_infos[i].queueCount = 1;
    if (indices[i] == context->device.graphics_queue_index) {
      queue_create_infos[i].queueCount = 2;
    }
    queue_create_infos[i].flags = 0;
    queue_create_infos[i].pNext = 0;
    float queue_priority = 1.0f;
    queue_create_infos[i].pQueuePriorities = &queue_priority;
  }

  // Request device features.
  // TODO: should be config driven
  VkPhysicalDeviceFeatures device_features = {};
  device_features.samplerAnisotropy = VK_TRUE; // Request anistrophy

  VkDeviceCreateInfo device_create_info = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_create_info.queueCreateInfoCount = index_count;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledExtensionCount = 1;
  const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  device_create_info.ppEnabledExtensionNames = &extension_names;

  // Deprecated and ignored, so pass nothing.
  device_create_info.enabledLayerCount = 0;
  device_create_info.ppEnabledLayerNames = 0;

  // Create the device.
  VK_CHECK(vkCreateDevice(context->device.physical_device, &device_create_info,
                          nullptr, &context->device.logical_device));

  OE_LOG(LOG_LEVEL_INFO, "Vulkan logical device created!");

  return true;
}

bool vulkan_device_create_logical_device(backend_context *context) {

  const float priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::vector<uint32_t> queue_family_indices = {
      static_cast<uint32_t>(context->device.graphics_queue_index),
      static_cast<uint32_t>(context->device.present_queue_index),
      static_cast<uint32_t>(context->device.transfer_queue_index)};
  // Create each queue
  for (auto queue : queue_family_indices) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue;
    queue_create_info.flags = 0;
    queue_create_info.pNext = 0;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &priority;
    queue_create_infos.push_back(queue_create_info);
  }
  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.queueCreateInfoCount = queue_create_infos.size();
  device_create_info.pEnabledFeatures = &context->device.features;
  const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  device_create_info.ppEnabledExtensionNames = &extension_names;

  // Add validation layers if in debug mode
#ifndef NDEBUG
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  OE_LOG(LOG_LEVEL_DEBUG, "Available layer count: %d", layerCount);
  std::vector<VkLayerProperties> availableLayerProperties(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount,
                                     availableLayerProperties.data());

  std::vector<const char *> availableLayers(layerCount);
  for (auto var : availableLayerProperties) {
    availableLayers.push_back(var.layerName);
  }
  device_create_info.enabledLayerCount = 0;
  device_create_info.ppEnabledLayerNames = 0;
#endif

  VK_CHECK(vkCreateDevice(context->device.physical_device, &device_create_info,
                          nullptr, &context->device.logical_device));

  OE_LOG(LOG_LEVEL_INFO, "Created logical device!");
  return true;
}
