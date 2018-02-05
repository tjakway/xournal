#pragma once

#include <glib.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "xournal.h"

#include "xo-map-to-output-error.h"
#include "xo-map-to-output.h"


gboolean map_to_output_mapped_rect_is_visible(MapToOutput*, MapToOutputError*);

/**
 * check that the rect outlining the mapped area matches the driver area's mapping
 * (need to check that they match in pixels within a certain margin
 */
gboolean map_to_output_mapping_is_correct(MapToOutput*, MapToOutputError*);

//map_to_output_mapped_rect_is_visible should return true after calling this
void map_to_output_scroll_to_mapped_rect(MapToOutput*, MapToOutputError*);

/**
 * returns the top left x, y, width and height of the drawing area of the canvas widget
 * (i.e. excluding the non-drawable background area)
 */
void map_to_output_get_canvas_drawing_area_dimensions(
        GnomeCanvas*, Page*, 
        double,
        gint*, gint*, double*, double*, 
        //optional boolean parameter to indicate whether the canvas is larger than our page
        //pass NULL if you don't care
        gboolean* no_gap,
        MapToOutputError*);
