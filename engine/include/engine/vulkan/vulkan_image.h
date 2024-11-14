#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <vulkan/vulkan_core.h>

#include "engine/renderer_types.inl"

void vulkan_image_copy_from_buffer(backend_context* context,
                                   vulkan_buffer buffer, vulkan_image image,
                                   uint32_t height, uint32_t width);

void vulkan_image_create(backend_context* context, uint32_t height,
                         uint32_t width, vk::Format format,
                         vk::Flags<vk::ImageUsageFlagBits> usage,
                         vulkan_image* out_image);

void vulkan_image_create_view(backend_context* context, vk::Format format,
                              vk::ImageAspectFlagBits flags, vk::Image* image,
                              vk::ImageView* out_image_view);

void vulkan_image_create_sampler(backend_context* context, vulkan_image* image,
                                 vk::Sampler* out_sampler);

void vulkan_image_transition_layout(backend_context* context,
                                    vulkan_image* image, vk::Format format,
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout);

#endif
