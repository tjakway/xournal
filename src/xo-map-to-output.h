#pragma once

#include <libgnomecanvas/libgnomecanvas.h>

typedef struct MapToOutputConfig {
    const char* driver_program;
    const char** driver_argv;
    const int driver_argc;

    //default: red
    guint line_color;
};

/****error types*****/
enum MapToOutputErrorType {
    NO_ERROR = 0,
    INIT_ERROR;
};

typedef struct MapToOutputError {
    enum MapToOutputErrorType err_type;
    char* err_msg;
};

MapToOutputError* map_to_output_err_new();
void map_to_output_err_free(MapToOutputError*);


/********************/

typedef struct MapToOutput {
    MapToOutputConfig* config;

    GnomeCanvasItem* left_line, 
        right_line, 
        top_line, 
        bottom_line;

    gdouble bottom_left_x, bottom_left_y,
            top_right_x, top_right_y;
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
 * if needs_new_page is set to true, call map_to_output_new_page immediately
 */
void map_to_output_shift_down(
        MapToOutputConfig*, 
        GnomeCanvas*,
        gboolean* needs_new_page,
        MapToOutputError*);

void map_to_output_new_page(
        MapToOutputConfig*, 
        GnomeCanvas*,
        MapToOutputError*);

void map_to_output_on_resize(
        MapToOutputConfig*, 
        GnomeCanvas*,
        MapToOutputError*);
