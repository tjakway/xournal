#pragma once

#include <libgnomecanvas/libgnomecanvas.h>

#include "xo-map-to-output-error.h"
#include "xo-tablet-driver.h"

typedef struct MapToOutputConfig {
    const char* driver_program;
    const char** driver_argv;
    const int driver_argc;

    //default: red
    guint line_color;

    TabletDriver driver;
} MapToOutputConfig;


//probably don't need these, just use statically
//allocated error structures
//MapToOutputError* map_to_output_err_new();
//void map_to_output_err_free(MapToOutputError*);


/********************/

typedef struct MapToOutput {
    MapToOutputConfig* config;

    GnomeCanvas* canvas;

    guint width, height;
    double pixels_per_unit;

    unsigned int tablet_width, tablet_height;

    GnomeCanvasItem* left_line, 
        right_line, 
        top_line, 
        bottom_line;

    gdouble bottom_left_x, bottom_left_y,
            top_right_x, top_right_y;
} MapToOutput;

//pass NULL for default config
MapToOutput* map_to_output_init(
        MapToOutputConfig*, 
        GnomeCanvas*,
        guint width,
        guint height,
        double pixels_per_unit,
        MapToOutputError*);

void map_to_output_free(
        MapToOutput*, 
        MapToOutputError*);

/**
 * if needs_new_page is set to true, call map_to_output_new_page immediately
 */
void map_to_output_shift_down(
        MapToOutput*, 
        gboolean* needs_new_page,
        MapToOutputError*);

void map_to_output_new_page(
        MapToOutput*, 
        MapToOutputError*);

void map_to_output_on_resize(
        MapToOutput*, 
        MapToOutputError*);
