#pragma once

#include "xo-map-to-output-error.h"

typedef struct TabletDriver {
    void (*get_tablet_dimensions)(unsigned int*, unsigned int*, MapToOutputError*);
    
    //converted to a string of the form WIDTHxHEIGHT+X+Y
    //see https://tronche.com/gui/x/xlib/utilities/XParseGeometry.html
    //for the X11 coordinate system, the origin is the top left and y increases going down
    void (*map_to_output)(unsigned int width, unsigned int height, 
            unsigned int x, unsigned int y, MapToOutputError*);

    //return to the default state
    void (*reset_map_to_output)(MapToOutputError*);
} TabletDriver;
