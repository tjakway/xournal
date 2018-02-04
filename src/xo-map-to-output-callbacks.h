#pragma once

#include <glib.h>

#include "xo-map-to-output.h"

//the callbacks need access to our data structures but we can't pass
//them so globals are the only option
extern MapToOutput* GLOBAL_MAP_TO_OUTPUT;
G_LOCK_EXTERN(GLOBAL_MAP_TO_OUTPUT);

void map_to_output_on_canvas_button_press();
