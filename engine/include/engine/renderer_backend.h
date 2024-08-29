#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "engine/asserts.h"

#include <vulkan/vulkan.h>

typedef struct backend_context {
  VkInstance instance;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

} backend_context;

bool renderer_backend_initialize();

void renderer_backend_shutdown();

#define VK_CHECK(expr) \
    do { \
        VkResult result = (expr); \
        OE_ASSERT(result == VK_SUCCESS); \
    } while(0)

#endif
