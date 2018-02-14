#pragma once

#include "xo-map-to-output.h"
#include "xo-map-to-output-error.h"

#include <assert.h>

#include <libgnomecanvas/libgnomecanvas.h>

/**
 * scroll to the output box or do nothing if it's already visible
 */
void scroll_to_output_box(
        MapToOutput* map_to_output, 
        GnomeCanvas* canvas,
        Page* page,
        double zoom,
        OutputBox output_box,
        MapToOutputError* err);
