#include "engine/vulkan/vulkan_pipeline.h"

#include <vulkan/vulkan_core.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

#include "engine/logger.h"
#include "engine/renderer_types.inl"

void create_descriptor_set(backend_context* context) {
  // UBO
  vk::DescriptorSetLayoutBinding ubo_layout_binding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .pImmutableSamplers = nullptr,
  };
  vk::DescriptorSetLayoutBinding sampler_layout_binding{
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr,
  };

  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      ubo_layout_binding, sampler_layout_binding};

  VkPipelineLayout pipeline_layout{};

  vk::DescriptorSetLayoutCreateInfo layout_info{
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data()};

  context->pipeline.descriptor_set_layout =
      context->device.logical_device.createDescriptorSetLayout(layout_info);
}

void vulkan_pipeline_create(backend_context* context,
                            vulkan_renderpass* renderpass,
                            vk::ShaderModule vert_shader,
                            vk::ShaderModule frag_shader,
                            vulkan_pipeline* out_pipeline) {
  // create descriptor sets
  create_descriptor_set(context);

  // Create the pipeline stages
  vk::PipelineShaderStageCreateInfo vss_info{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vert_shader,
      .pName = "main",
  };  // vert shader stage(vss) create info.
  vk::PipelineShaderStageCreateInfo fss_info{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = frag_shader,
      .pName = "main"};
  // Bundle together
  vk::PipelineShaderStageCreateInfo shader_stages[] = {vss_info, fss_info};

  std::vector<vk::DynamicState> dynamic_states{vk::DynamicState::eViewport,
                                               vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamic_state{
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data(),
  };
  // get vertex descriptions
  auto binding_description = Vertex::get_binding_description();
  auto attrib_description = Vertex::get_attribute_descriptions();

  vk::PipelineVertexInputStateCreateInfo vertex_input_ci{
      .vertexBindingDescriptionCount = binding_description.size(),
      .pVertexBindingDescriptions = binding_description.data(),
      .vertexAttributeDescriptionCount = attrib_description.size(),
      .pVertexAttributeDescriptions = attrib_description.data()};

  // We're triangle gamers here
  vk::PipelineInputAssemblyStateCreateInfo input_assembly{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = vk::False};

  vk::Viewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = (float)context->swapchain.extent.width,
                        .height = (float)context->swapchain.extent.height,
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};

  vk::Rect2D scissor{.offset = {.x = 0, .y = 0},
                     .extent = context->swapchain.extent};

  vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1,
                                                     .pViewports = &viewport,
                                                     .scissorCount = 1,
                                                     .pScissors = &scissor};

  // Rasterizer time. It'd be cool to beat Epic games' nanite someday

  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = vk::False,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f,
  };
  // TODO: Look at VK_POLYGON_MODE_LINE/POINT for
  // editor
  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = vk::False,
      .alphaToOneEnable = vk::False};

  vk::PipelineColorBlendAttachmentState color_blend_attachment{
      .blendEnable = vk::False,
      .srcColorBlendFactor = vk::BlendFactor::eOne,
      .dstColorBlendFactor = vk::BlendFactor::eZero,
      .colorBlendOp = vk::BlendOp::eAdd,
      .srcAlphaBlendFactor = vk::BlendFactor::eOne,
      .dstAlphaBlendFactor = vk::BlendFactor::eZero,
      .alphaBlendOp = vk::BlendOp::eAdd,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
  };
  std::array<float, 4> blend_constants_array = {0.0f, 0.0f, 0.0f, 0.0f};
  vk::ArrayWrapper1D<float, 4> blend_constants(blend_constants_array);

  vk::PipelineColorBlendStateCreateInfo color_blending{
      .logicOpEnable = vk::False,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants = blend_constants,
  };
  // Make the pipeline layout, this affects our uniforms (IE our VP part of
  // our MVP)
  vk::PipelineLayoutCreateInfo pipeline_layout_infos{

      .setLayoutCount = 1,
      .pSetLayouts = &context->pipeline.descriptor_set_layout,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr};

  vk::PipelineLayoutCreateInfo pipeline_layout_info{
      .setLayoutCount = 1,
      .pSetLayouts = &context->pipeline.descriptor_set_layout,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };

  out_pipeline->layout =
      context->device.logical_device.createPipelineLayout(pipeline_layout_info);

  vk::GraphicsPipelineCreateInfo pipeline_create_info{
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_ci,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = nullptr,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state,
      .layout = out_pipeline->layout,
      .renderPass = renderpass->handle,
      .subpass = 0,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = -1,
  };

  out_pipeline->handle = context->device.logical_device.createGraphicsPipeline(
      nullptr, pipeline_create_info);

  OE_LOG(LOG_LEVEL_INFO, "Created graphics pipeline");

  // Not needed after bound to pipeline
  vkDestroyShaderModule(context->device.logical_device, vert_shader, nullptr);
  vkDestroyShaderModule(context->device.logical_device, frag_shader, nullptr);

  return;
}
