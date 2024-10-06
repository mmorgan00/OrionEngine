#ifndef APPLICATION_H
#define APPLICATION_H

#include "engine/platform.h"

void application_initialize(platform_state* plat_state);

bool application_run();

void application_shutdown();

#endif
