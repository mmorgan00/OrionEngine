#include "engine/vulkan/vulkan_pipeline.h"

#include <vulkan/vulkan_core.h>

#include "engine/logger.h"
#include "engine/renderer_types.inl"

void create_descriptor_set(backend_context* context) {
  // UBO
  VkDescriptorSetLayoutBinding ubo_layout_binding{};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  ubo_layout_binding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding = 1;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
      ubo_layout_binding, sampler_layout_binding};

  VkPipelineLayout pipeline_layout{};

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
  layout_info.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(
      context->device.logical_device, &layout_info, nullptr,
      &context->pipeline.descriptor_set_layout));
}

void vulkan_pipeline_create(backend_context* context,
                            vulkan_renderpass* renderpass,
                            VkShaderModule vert_shader,
                            VkShaderModule frag_shader,
                            vulkan_pipeline* out_pipeline) {
  // create descriptor sets
  create_descriptor_set(context);

  // Create the pipeline stages
  VkPipelineShaderStageCreateInfo
      vss_info{};  // vert shader stage(vss) create info.
  vss_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  ;
  vss_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vss_info.module = vert_shader;
  vss_info.pName = "main";  // Don't be weird and change this to something that
                            // isn't main. It'll break more than it'll help
  vss_info.pSpecializationInfo = nullptr;  // REVISIT THIS AT SOME POINT.

  VkPipelineShaderStageCreateInfo fss_info{};
  fss_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fss_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fss_info.module = frag_shader;
  fss_info.pName = "main";

  // Bundle together
  VkPipelineShaderStageCreateInfo shader_stages[] = {vss_info, fss_info};

  std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount =
      static_cast<uint32_t>(dynamic_states.size());
  dynamic_state.pDynamicStates = dynamic_states.data();

  // get vertex descriptions
  auto binding_description = Vertex::get_binding_description();
  auto attrib_description = Vertex::get_attribute_descriptions();

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attrib_description.size());
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = attrib_description.data();
  // We're triangle gamers here
  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)context->swapchain.extent.width;
  viewport.height = (float)context->swapchain.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = context->swapchain.extent;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;
  viewport_state.pViewports = &viewport;

  // Rasterizer time. It'd be cool to beat Epic games' nanite someday
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode =
      VK_POLYGON_MODE_FILL;     // TODO: Look at VK_POLYGON_MODE_LINE/POINT for
                                // editor
  rasterizer.lineWidth = 1.0f;  // Other than this requires GPU feature
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;           // Optional
  multisampling.pSampleMask = nullptr;             // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
  multisampling.alphaToOneEnable = VK_FALSE;       // Optional

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ZERO;                                          // Optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  color_blend_attachment.dstAlphaBlendFactor =
      VK_BLEND_FACTOR_ZERO;                               // Optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional
  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;  // Optional
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;  // Optional
  color_blending.blendConstants[1] = 0.0f;  // Optional
  color_blending.blendConstants[2] = 0.0f;  // Optional
  color_blending.blendConstants[3] = 0.0f;
  // Make the pipeline layout, this affects our uniforms (IE our VP part of our
  // MVP)

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &context->pipeline.descriptor_set_layout;
  pipeline_layout_info.pushConstantRangeCount = 0;
  pipeline_layout_info.pPushConstantRanges = nullptr;

  VK_CHECK(vkCreatePipelineLayout(context->device.logical_device,
                                  &pipeline_layout_info, nullptr,
                                  &out_pipeline->layout));

  VkGraphicsPipelineCreateInfo pipeline_create_info{};
  pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.stageCount =
      2;  // TODO: Configure based on actual shaders passed in?
  pipeline_create_info.pStages = shader_stages;
  pipeline_create_info.pVertexInputState = &vertex_input_info;
  pipeline_create_info.pInputAssemblyState = &input_assembly;
  pipeline_create_info.pViewportState = &viewport_state;
  pipeline_create_info.pRasterizationState = &rasterizer;
  pipeline_create_info.pMultisampleState = &multisampling;
  pipeline_create_info.pDepthStencilState = nullptr;
  pipeline_create_info.pColorBlendState = &color_blending;
  pipeline_create_info.pDynamicState = &dynamic_state;
  pipeline_create_info.layout = out_pipeline->layout;
  pipeline_create_info.renderPass = renderpass->handle;
  pipeline_create_info.subpass = 0;  // index
  pipeline_create_info.basePipelineHandle = nullptr;
  pipeline_create_info.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(context->device.logical_device,
                                     VK_NULL_HANDLE, 1, &pipeline_create_info,
                                     nullptr, &out_pipeline->handle));
  OE_LOG(LOG_LEVEL_INFO, "Created graphics pipeline");

  // Not needed after bound to pipeline
  vkDestroyShaderModule(context->device.logical_device, vert_shader, nullptr);
  vkDestroyShaderModule(context->device.logical_device, frag_shader, nullptr);

  return;
}
