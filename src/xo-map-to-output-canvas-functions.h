#pragma once

#include <glib.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "xo-map-to-output-error.h"
#include "xo-map-to-output.h"


gboolean map_to_output_mapped_rect_is_visible(MapToOutput*, MapToOutputError*);

/**
 * check that the rect outlining the mapped area matches the driver area's mapping
 * (need to check that they match in pixels within a certain margin
 */
gboolean map_to_output_mapping_is_correct(MapToOutput*, MapToOutputError*);
