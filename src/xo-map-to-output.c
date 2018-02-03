#include "xo-map-to-output.h"

#include <stdlib.h>

//TODO: fill in default config
static MapToOutputConfig default_config;


typedef struct OutputBox {
    double top_left_x, top_left_y;
    guint width, height;
} OutputBox;

static OutputBox calculate_output_box(
        double top_left_x,
        double top_left_y,
        guint canvas_width,
        guint canvas_height,
        unsigned int tablet_width, 
        unsigned int tablet_height,
        gboolean* needs_new_page)
{
    const double tablet_aspect_ratio  = 
        ((double)tablet_width) / ((double)tablet_height);

    OutputBox output_box;

    output_box.width = canvas_width;
    output_box.height = tablet_aspect_ratio * canvas_height;
    output_box.top_left_x = top_left_x;
    output_box.top_left_y = top_left_y;

    const guint max_height = top_left_y + canvas_height;
    if((top_left_y + output_box_height) > max_height)
    {
        *needs_new_page = true;
    }
    else
    {
        *needs_new_page = false;
    }

    return output_box;
}

static void output_box_to_lines(OutputBox output_box,
        double* top, 
        double* right,
        double* bottom,
        double* left)
{
    const double[] top_left = { output_box.top_left_x, output_box.top_left_y };

    const double[] top_right = 
            { output_box.top_left_x + output_box.width, output_box.top_left_y };

    const double[] bottom_left = 
            { output_box.top_left_x, 
                output_box.top_left_y  + output_box.height };

    const double[] bottom_right = 
            { output_box.top_left_x + output_box.width, 
                output_box.top_left_y  + output_box.height };


    //top point 1 = top left
    top[0] = top_left[0];
    top[1] = top_left[1];

    //top point 2 = top right
    top[2] = top_right[0];
    top[3] = top_right[1];

    
    //right point 1 = top right
    right[0] = top_right[0];
    right[1] = top_right[1];

    //right point 2 = bottom right
    right[2] = bottom_right[0];
    right[3] = bottom_right[1];


    //bottom point 1 = bottom left
    bottom[0] = bottom_left[0];
    bottom[1] = bottom_left[1];

    //bottom point 2 = bottom right
    bottom[2] = bottom_right[0];
    bottom[3] = bottom_right[1];

    
    //left point 1 = top left
    left[0] = top_left[0];
    left[1] = top_left[1];

    //left point 2 = bottom left
    left[2] = bottom_left[0];
    left[3] = bottom_left[1];
}




MapToOutput* map_to_output_init(
        MapToOutputConfig* config_param, 
        GnomeCanvas* canvas,
        guint width,
        guint height,
        MapToOutputError* err)
{
    MapToOutputConfig* config;
    if(config_param == NULL)
    {
        config = &default_config;
    }
    else
    {
        config = config_param;
    }

    MapToOutput* map_to_output = malloc(sizeof(MapToOutput));
    if(map_to_output == NULL)
    {
        *err = bad_malloc;
        return NULL;
    }


    //need the tablet dimensions to calculate the aspect ratio
    map_to_output->config.get_table_dimensions(
            &map_to_output->tablet_width,
            &map_to_output->tablet_height,
            err);

    if(err->err_type != NO_ERROR)
    {
        return NULL;
    }
}
