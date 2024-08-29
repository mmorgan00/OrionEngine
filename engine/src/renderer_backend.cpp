
#include "engine/renderer_backend.h"
#include "engine/logger.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static backend_context context; 

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

bool check_validation_layer_support();


bool renderer_backend_initialize() {

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
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  // Create a vector to hold the extension names
  std::vector<const char*> extensionNames;
  for (const auto& extension : extensions) {
    extensionNames.push_back(extension.extensionName); // Add the extension name to the vector
  }
  OE_LOG(LOG_LEVEL_DEBUG, "available extensions:\n");

  for (const auto& extension : extensionNames) {
    OE_LOG(LOG_LEVEL_DEBUG, "\t%s\n", extension);
  }

#ifdef NDEBUG

  extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensionNames.data();


  VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
  if(result != VK_SUCCESS){
    OE_LOG(LOG_LEVEL_FATAL, "Failed to initialize Vulkan instance!");
  } 

  if(!check_validation_layer_support()){
    OE_LOG(LOG_LEVEL_FATAL, "Failed to get validation layers!");
    return false;
  }

  // Debugger
#ifdef NDEBUG
  OE_LOG(LOG_LEVEL_INFO,"Creating Vulkan debugger...\n");

  uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; //|
                                                  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

  debug_create_info.messageSeverity = log_severity;
  debug_create_info.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug_create_info.pfnUserCallback = vk_debug_callback;

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        context.instance, "vkCreateDebugUtilsMessengerEXT");
  if(func == nullptr) {
    OE_LOG(LOG_LEVEL_ERROR, "Failed to create debug messenger");
    return false;
  }
  if(VK_SUCCESS != func(context.instance, &debug_create_info, nullptr, &context.debug_messenger)){
    
  }

  OE_LOG(LOG_LEVEL_DEBUG,"Vulkan debugger created.\n");
#endif 
  return true; 
} 

void renderer_backend_shutdown(){ 
  vkDestroyInstance(context.instance, nullptr); 
} 

bool check_validation_layer_support(){ 
  // Validation layers.
  const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
  OE_LOG(LOG_LEVEL_DEBUG, "Validation layers enabled. Enumerating...\n");

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  OE_LOG(LOG_LEVEL_DEBUG, "Available layer count: %d\n", layerCount);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  OE_LOG(LOG_LEVEL_DEBUG, "Available layers: \n");
  for (const auto& layerProperties : availableLayers) {
    OE_LOG(LOG_LEVEL_DEBUG, "\t%s\n", layerProperties.layerName);
  }

  for(const char* layerName: validationLayers) {
    bool layerFound = false;
    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        OE_LOG(LOG_LEVEL_DEBUG, "%s layer found\n", layerName);
        break;
      }

    }
    if(!layerFound) {
      return false;
    }
  }
  OE_LOG(LOG_LEVEL_DEBUG, "All required validation layers are present.\n");

#endif
  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data) {

  // TODO: Check Error level for verbosity reasons
  OE_LOG(LOG_LEVEL_VK_ERROR, "Validation error: %s", callback_data->pMessage);
  return false;
}


