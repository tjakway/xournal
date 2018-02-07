#pragma once

#include <libgnomecanvas/libgnomecanvas.h>

#include "xo-map-to-output-decl.h"
#include "xo-map-to-output-error.h"
#include "xo-tablet-driver.h"

struct MapToOutputConfig {
    //default: red
    guint line_color;

    double line_width_units;

    //allowed mismatch between our outlined rect and 
    //the screen region the driver is mapping output to
    //(in pixels)
    guint mapping_allowed_error;

    TabletDriver driver;
};

MapToOutputConfig get_default_config();


/********************/

struct OutputBox {
    double top_left_x, top_left_y;
    guint width, height;
};

struct MapToOutput {
    MapToOutputConfig* config;

    GnomeCanvas* canvas;

    guint width, height;
    double pixels_per_unit;

    unsigned int tablet_width, tablet_height;

    GnomeCanvasItem *left_line, 
        *right_line, 
        *top_line, 
        *bottom_line;

    //NULL if mapping_mode == NO_MAPPING
    OutputBox* output_box;

    enum MapToOutputMode {
        NO_MAPPING = 0,
        MAPPING_ACTIVE
    } mapping_mode;

};

//pass NULL for default config
MapToOutput* map_to_output_init(
        MapToOutputConfig*, 
        GnomeCanvas*,
        MapToOutputError*);

void map_to_output_free(
        MapToOutput*, 
        MapToOutputError*);

/**
 * if needs_new_page is set to true, make a new page and call map_to_output_new_page
 */
void map_to_output_shift_down(
        MapToOutput*, 
        gboolean* needs_new_page,
        MapToOutputError*);

void map_to_output_new_page(
        MapToOutput*, 
        MapToOutputError*);

//sanity checks for MapToOutput
//uses assert so no need for a MapToOutputError* parameter
void map_to_output_asserts(MapToOutput*);

//enable/disable MapToOutput functionality
//should restore regular pointer behavior when disabled
void map_to_output_enable(gboolean);
void map_to_output_toggle(gboolean);
