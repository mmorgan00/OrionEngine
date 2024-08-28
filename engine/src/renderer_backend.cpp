
#include "engine/renderer_backend.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static backend_context* context; 

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

bool checkValidationLayerSupport();


bool renderer_backend_initialize() {

  context = (backend_context*)malloc(sizeof(backend_context));

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
  std::cout << "available extensions:\n";

  for (const auto& extension : extensionNames) {
    std::cout << '\t' << extension << '\n';
  }

  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensionNames.data();


  VkInstance instance;
  VkResult result = vkCreateInstance(&createInfo, nullptr, &context->instance);
  if(result != VK_SUCCESS){
    std::cout << "Failed to initialize Vulkan instance!" << std::endl;
  } 

  if(!checkValidationLayerSupport()){
    std::cout << "Failed to get validation layers\n";
    return false;
  }
  /**
  // Debugger
#ifdef NDEBUG
std::cout << "Creating Vulkan debugger...\n";
uint32_t logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; //|
                                              // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

                                              VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
                                              VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
                                              debugCreateInfo.messageSeverity = log_severity;
                                              debugCreateInfo.messageType =
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                                              debugCreateInfo.pfnUserCallback = vk_debug_callback;

                                              PFN_vkCreateDebugUtilsMessengerEXT func =
                                              (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                              context.instance, "vkCreateDebugUtilsMessengerEXT");
                                              OASSERT_MSG(func, "Failed to create debug messenger!");
                                              VK_CHECK(func(context.instance, &debug_create_info, context.allocator,
                                              &context.debug_messenger));
                                              std::count << "Vulkan debugger created.\n";
#endif
   **/
  return true;
}

void renderer_backend_shutdown(){
  vkDestroyInstance(context->instance, nullptr);
}


bool checkValidationLayerSupport(){
  // Validation layers.
  const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG

  std::cout << "Validation layers enabled. Enumerating...\n";

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::cout << "Available layer count: " << layerCount << "\n";
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  std::cout << "Available layers: " << availableLayers.data() << "\n";

  for(const char* layerName: validationLayers) {
    bool layerFound = false;
    for (const auto& layerProperties : availableLayers) {
      std::cout << layerProperties.layerName << "\n";
      std::cout << layerName << "\n";
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
      
    }
    if(!layerFound) {
      return false;
    }
  }
      std::cout << "All required validation layers are present.\n";

#endif
      return true;
    }
