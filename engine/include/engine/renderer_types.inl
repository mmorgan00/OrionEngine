#ifndef RENDERER_TYPES
#define RENDERER_TYPES

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "engine/asserts.h"

// Renderer 'primitives'

typedef struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
} ubo;

// TODO: Just doing this for convenience for now. Vertex shouldn't be color data
typedef struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_description;
  }
  static std::array<VkVertexInputAttributeDescription, 3>
  get_attribute_descriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    // TODO: Change to tex coords
    // color
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

    return attribute_descriptions;
  }
} Vertex;

// TODO: REMOVE THIS. WE JUST WANT TO DRAW SOME GEOMETRY FOR NOW. /MAYBE/ we'll
// keep it for texture coords interleaved but big if
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
// TODO: Move these all to a 'vulkan types'. We should abstract more so we can
// support other APIs potentially
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct vulkan_image {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
} vulkan_image;

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
  VkDescriptorSetLayout descriptor_set_layout{};
} vulkan_pipeline;

typedef struct vulkan_shader_stage {
  VkShaderModuleCreateInfo create_info;
  VkShaderModule handle;
  VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

#define OBJECT_SHADER_STAGE_COUNT 2  // vertex, fragment for now

typedef struct vulkan_buffer {
  VkBuffer handle;
  VkDeviceMemory memory;
} vulkan_buffer;

typedef struct vulkan_object_shader {
  // vertex, fragment
  vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];

} vulkan_object_shader;

typedef struct vulkan_texture {
  vulkan_image image;
  VkSampler sampler;
} vulkan_texture;

typedef struct backend_context {
  VkInstance instance;
  vulkan_device device;
  GLFWwindow* window;
  VkSurfaceKHR surface;
  vulkan_pipeline pipeline;
  vulkan_renderpass main_renderpass;
  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffer;
  vulkan_buffer vert_buff;
  vulkan_buffer index_buff;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
  vulkan_swapchain swapchain;
  std::vector<VkSemaphore> image_available_semaphore;
  std::vector<VkSemaphore> render_finished_semaphore;
  std::vector<VkFence> in_flight_fence;
  uint32_t current_frame;
  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;
  std::vector<vulkan_buffer> uniform_buffers;
  std::vector<void*> uniform_buffer_memory;
  vulkan_texture default_texture;
} backend_context;

#define VK_CHECK(expr)               \
  do {                               \
    VkResult result = (expr);        \
    OE_ASSERT(result == VK_SUCCESS); \
  } while (0)

#endif
