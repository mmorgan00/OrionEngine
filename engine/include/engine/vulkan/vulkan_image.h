#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <vulkan/vulkan_core.h>

#include "engine/renderer_types.inl"

void vulkan_image_copy_from_buffer(backend_context* context,
                                   vulkan_buffer buffer, vulkan_image image,
                                   uint32_t height, uint32_t width);

void vulkan_image_create(backend_context* context, uint32_t height,
                         uint32_t width, vulkan_image* out_image);

void vulkan_image_create_view(backend_context* context, VkFormat format,
                              VkImage* image, VkImageView* out_image_view);

void vulkan_image_create_sampler(backend_context* context, vulkan_image* image,
                                 VkSampler* out_sampler);

void vulkan_image_transition_layout(backend_context* context,
                                    vulkan_image* image, VkFormat format,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout);

#endif
