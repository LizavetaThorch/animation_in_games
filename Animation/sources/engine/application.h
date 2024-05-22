#pragma once
#include "log.h"
#include "input.h"

void init_application(const char *project_name, int width, int height, bool full_screen);

void close_application();

void main_loop();

float get_aspect_ratio();
