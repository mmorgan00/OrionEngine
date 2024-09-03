#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "engine/renderer_types.inl"

bool vulkan_device_create(backend_context* context);

bool vulkan_device_create_logical_device(backend_context* context);

#endif
