#ifndef RENDERER_TYPES
#define RENDERER_TYPES

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "engine/asserts.h"

// Renderer 'primitives'

typedef struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
} ubo;

// TODO: Just doing this for convenience for now. Vertex shouldn't be color data
typedef struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static std::array<vk::VertexInputBindingDescription, 1>
  get_binding_description() {
    vk::VertexInputBindingDescription binding_desc{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
    std::array<vk::VertexInputBindingDescription, 1> binding = {{binding_desc}};
    return binding;
  };
  static std::array<vk::VertexInputAttributeDescription, 3>
  get_attribute_descriptions() {
    vk::VertexInputAttributeDescription vert_desc = {
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, pos)};

    vk::VertexInputAttributeDescription color_desc = {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, color)};

    vk::VertexInputAttributeDescription tex_desc = {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, tex_coord)};

    std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions{
        vert_desc, color_desc, tex_desc

    };
    return attribute_descriptions;
  }
} Vertex;

// TODO: REMOVE THIS. WE JUST WANT TO DRAW SOME GEOMETRY FOR NOW. /MAYBE/ we'll
// keep it for texture coords interleaved but big if
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

const std::vector<uint16_t> indices = {4, 5, 6, 6, 7, 4, 0, 1, 2, 2, 3, 0};
// TODO: Move these all to a 'vulkan types'. We should
// abstract more so we can
// support other APIs potentially
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct vulkan_image {
  vk::Image handle;
  vk::DeviceMemory memory;
  vk::ImageView view;
} vulkan_image;

typedef struct vulkan_swapchain_support_info {
  vk::SurfaceCapabilitiesKHR capabilities;
  uint32_t format_count;
  std::vector<vk::SurfaceFormatKHR> formats;
  uint32_t present_mode_count;
  vk::PresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
  vk::PhysicalDevice physical_device;
  vk::Device logical_device;
  vulkan_swapchain_support_info swapchain_support;
  int graphics_queue_index;
  int present_queue_index;
  int transfer_queue_index;

  vk::Queue graphics_queue;
  vk::Queue present_queue;
  vk::Queue transfer_queue;

  VkCommandPool graphics_command_pool;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_renderpass {
  vk::RenderPass handle;
} vulkan_renderpass;

typedef struct vulkan_swapchain {
  vk::Format image_format;
  vk::SwapchainKHR handle;
  vk::Extent2D extent;
  uint32_t image_count;
  std::vector<vk::Image> images;
  std::vector<vk::ImageView> views;  // images not accessed directly in Vulkan
  std::vector<vk::Framebuffer> framebuffers;
} vulkan_swapchain;

typedef struct vulkan_pipeline {
  vk::Pipeline handle;
  vk::PipelineLayout layout;
  vk::DescriptorSetLayout descriptor_set_layout{};
} vulkan_pipeline;

typedef struct vulkan_shader_stage {
  vk::ShaderModuleCreateInfo create_info;
  vk::ShaderModule handle;
  vk::PipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

#define OBJECT_SHADER_STAGE_COUNT 2  // vertex, fragment for now

typedef struct vulkan_buffer {
  vk::Buffer handle;
  vk::DeviceMemory memory;
} vulkan_buffer;

typedef struct vulkan_object_shader {
  // vertex, fragment
  vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];

} vulkan_object_shader;

typedef struct vulkan_texture {
  vulkan_image image;
  vk::Sampler sampler;
} vulkan_texture;

typedef struct backend_context {
  vk::UniqueInstance instance;
  vulkan_device device;
  GLFWwindow* window;
  vk::SurfaceKHR surface;
  vulkan_pipeline pipeline;
  vulkan_renderpass main_renderpass;
  vk::CommandPool command_pool;
  std::vector<vk::CommandBuffer> command_buffer;
  vulkan_buffer vert_buff;
  vulkan_buffer index_buff;
#ifndef NDEBUG
  vk::DebugUtilsMessengerEXT debug_messenger;
#endif
  vulkan_swapchain swapchain;
  std::vector<vk::Semaphore> image_available_semaphore;
  std::vector<vk::Semaphore> render_finished_semaphore;
  std::vector<vk::Fence> in_flight_fence;
  uint32_t current_frame;
  vulkan_object_shader object_shader;
  vk::DescriptorPool descriptor_pool;
  std::vector<vk::DescriptorSet> descriptor_sets;
  std::vector<vulkan_buffer> uniform_buffers;
  std::vector<void*> uniform_buffer_memory;
  vulkan_texture default_texture;
} backend_context;

#define VK_CHECK(expr)                         \
  do {                                         \
    vk::Result result = (expr);                \
    OE_ASSERT(result == vk::Result::eSuccess); \
  } while (0)

#define VK_OBJECT_CREATE_CHECK(object) \
  do {                                 \
    OE_ASSERT(object != nullptr);      \
  } while (0)

#endif
