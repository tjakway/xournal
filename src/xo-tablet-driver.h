#pragma once

#include "xo-map-to-output-error.h"
#include "xo-map-to-output-decl.h"

#include <glib.h>

/**
 * the void* is for the driver to store its data structures
 */
typedef struct TabletDriver {
    void* (*init_driver)(MapToOutputError*);
    void (*free_driver)(void*);

    void (*get_tablet_dimensions)(void*, int*, int*, MapToOutputError*);
    
    //converted to a string of the form WIDTHxHEIGHT+X+Y
    //see https://tronche.com/gui/x/xlib/utilities/XParseGeometry.html
    //for the X11 coordinate system, the origin is the top left and y increases going down
    //
    //  **XXX** don't forget to set MapToOutput->mapping_mode to MAPPING_ACTIVE
    void (*map_to_output)(void*, unsigned int width, unsigned int height, 
            unsigned int x, unsigned int y, MapToOutputError*);

    //return to the default state
    //should:
    //  1. set mapping_mode to NO_MAPPING
    //  2. free MapToOutput->output_box
    //  3. set MapToOutput->output_box to NULL
    void (*reset_map_to_output)(void*, MapToOutputError*);
} TabletDriver;

extern const TabletDriver wacom_driver;
