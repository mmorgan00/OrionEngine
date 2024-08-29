
#include <vulkan/vulkan.h>

typedef struct backend_context {
  VkInstance instance;
#ifdef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

} backend_context;

bool renderer_backend_initialize();

void renderer_backend_shutdown();
