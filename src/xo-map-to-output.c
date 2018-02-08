#include "xo-map-to-output.h"
#include "xo-map-to-output-callbacks.h"
#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"

#include <stdlib.h>
#include <assert.h>

/**********************/
/*******GLOBALS********/
/**********************/
//allocate the globals forward-declared in xo-map-to-output-callbacks.h
MapToOutput* GLOBAL_MAP_TO_OUTPUT;
G_LOCK_DEFINE(GLOBAL_MAP_TO_OUTPUT);
/**********************/

//XXX: keep up to date
MapToOutputConfig get_default_config()
{
    return (MapToOutputConfig){
        .line_color = 0xFF0000FF,
        .line_width_units = 10,
        .mapping_allowed_error = 5,
        .driver = wacom_driver
    };
}

static OutputBox calculate_output_box(
        double top_left_x,
        double top_left_y,
        int canvas_width,
        int canvas_height,
        double tablet_aspect_ratio,
        gboolean* needs_new_page)
{
    OutputBox output_box;

    output_box.width = canvas_width;
    output_box.height = (int)(((double)canvas_width) / tablet_aspect_ratio);
    output_box.top_left_x = top_left_x;
    output_box.top_left_y = top_left_y;

    const int max_height = (int)(top_left_y + ((double)canvas_height));
    if((top_left_y + output_box.height) > max_height)
    {
        *needs_new_page = TRUE;
    }
    else
    {
        *needs_new_page = FALSE;
    }

    return output_box;
}

static void output_box_to_lines(OutputBox output_box,
        double* top, 
        double* right,
        double* bottom,
        double* left)
{
    const double top_left[] = { output_box.top_left_x, output_box.top_left_y };

    const double top_right[] = 
            { output_box.top_left_x + output_box.width, output_box.top_left_y };

    const double bottom_left[] = 
            { output_box.top_left_x, 
                output_box.top_left_y  + output_box.height };

    const double bottom_right[] = 
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

static void make_output_box_lines(
        MapToOutput* output,
        OutputBox output_box,
        GnomeCanvas* canvas,
        MapToOutputError* err)
{

    GnomeCanvasPoints *top_line_points = gnome_canvas_points_new(2),
                      *right_line_points = gnome_canvas_points_new(2),
                      *bottom_line_points = gnome_canvas_points_new(2),
                      *left_line_points = gnome_canvas_points_new(2);

    if(top_line_points == NULL
            || right_line_points == NULL
            || bottom_line_points == NULL
            || left_line_points == NULL)
    {
        static MapToOutputError null_points_error = {
            .err_type = CANVAS_INIT_ERROR,
            .err_msg = "gnome_canvas_points_new(2) returned NULL"
        };

        *err = null_points_error;
    }

    output_box_to_lines(output_box, 
            top_line_points->coords,
            right_line_points->coords,
            bottom_line_points->coords,
            left_line_points->coords);

    const guint line_color = output->config->line_color;
    const double line_width_units = output->config->line_width_units;

    output->top_line = gnome_canvas_item_new(
          gnome_canvas_root(canvas), 
          gnome_canvas_line_get_type(),
          "points", top_line_points,
          "fill-color-rgba", line_color,
          "width-units", line_width_units,
          NULL);

    output->right_line = gnome_canvas_item_new(
          gnome_canvas_root(canvas), 
          gnome_canvas_line_get_type(),
          "points", right_line_points,
          "fill-color-rgba", line_color,
          "width-units", line_width_units,
          NULL);

    output->bottom_line = gnome_canvas_item_new(
          gnome_canvas_root(canvas), 
          gnome_canvas_line_get_type(),
          "points", bottom_line_points,
          "fill-color-rgba", line_color,
          "width-units", line_width_units,
          NULL);

    output->left_line = gnome_canvas_item_new(
          gnome_canvas_root(canvas), 
          gnome_canvas_line_get_type(),
          "points", left_line_points,
          "fill-color-rgba", line_color,
          "width-units", line_width_units,
          NULL);

    if(output->top_line == NULL 
            || output->right_line == NULL
            || output->bottom_line == NULL
            || output->left_line == NULL)
    {
        static MapToOutputError null_item_error = {
            .err_type = CANVAS_INIT_ERROR,
            .err_msg = "gnome_canvas_item_new(2) returned NULL" 
        };

        *err = null_item_error;
    }
}


static void change_mapping(
        MapToOutput* map_to_output,
        GnomeCanvas* canvas,
        double new_top_left_x, double new_top_left_y,
        gboolean* needs_new_page,
        MapToOutputError* err)
{
    const double canvas_width = GTK_WIDGET(canvas)->allocation.width,
        canvas_height = GTK_WIDGET(canvas)->allocation.height;

    const double tablet_aspect_ratio = 
        ((double)map_to_output->tablet_width) / ((double)map_to_output->tablet_height);

    OutputBox output_box = calculate_output_box(
            //blank canvas, therefore a new page
            //so the top left is the origin
            new_top_left_x, new_top_left_y, 
            //canvas dimensions
            //TODO: should this be canvas allocation width and height?
            (int)canvas_width, (int)canvas_height,
            //tablet dimensions
            tablet_aspect_ratio,
            needs_new_page);

    if(*needs_new_page)
    {
        //TODO: can't need a new page before we've even started
        //probably means our window is too small
        err->err_type = NEED_NEW_PAGE;
        err->err_msg = "Need a new page, try zooming out more";

        map_to_output_free(map_to_output);
        return;
    }


    //call the tablet driver and map the tablet's output to our canvas region
    map_to_output->config->driver.map_to_output(
            map_to_output->driver_data,
            map_to_output,
            (unsigned int)output_box.top_left_x, 
            (unsigned int)output_box.top_left_y,
            (unsigned int)output_box.width,
            (unsigned int)output_box.height,
            err);
    if(err->err_type != NO_ERROR)
    {
        //TODO
    }
    else
    {
        //should have an active mapping after calling the driver
        assert(map_to_output->mapping_mode == MAPPING_ACTIVE);
    }

    //outline the mapped output box so the user can see it
    make_output_box_lines(map_to_output, output_box, canvas, err);
    if(err->err_type != NO_ERROR)
    {
        //reset the output mapping before returning
        MapToOutputError reset_err = no_error;
        map_to_output->config->driver.reset_map_to_output(
                map_to_output->driver_data, map_to_output, &reset_err);
        map_to_output_warn_if_error(&reset_err);
        
    }
}

//allocate the struct and set default values
//return NULL on failure
static MapToOutput* alloc_map_to_output()
{
    MapToOutput* map_to_output = malloc(sizeof(MapToOutput));
    if(map_to_output != NULL)
    {
        //null out all fields
        //see https://stackoverflow.com/questions/34958467/how-to-initialize-all-the-pointer-fields-of-struct-pointer-to-null
        //for an explanation of this syntax
        *map_to_output = (struct MapToOutput){ 0 };

        map_to_output->pixels_per_unit = -1.0;

        //no mapping active yet
        map_to_output->mapping_mode = NO_MAPPING;
    }
    return map_to_output;
}


MapToOutput* init_map_to_output(
        MapToOutputConfig* config_param, 
        GnomeCanvas* canvas,
        MapToOutputError* err)
{
    MapToOutputConfig* config;
    if(config_param == NULL)
    {
        //allocate a new config and make it a copy of the default config
        config = malloc(sizeof(MapToOutputConfig));
        if(config == NULL)
        {
            *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
        }
        else
        {
            *config = get_default_config();
        }
    }
    else
    {
        config = config_param;
    }

    MapToOutput* map_to_output = alloc_map_to_output();
    if(map_to_output == NULL)
    {
        *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
        goto map_to_output_init_error;
    }

    map_to_output->config = config;

    map_to_output->driver_data = map_to_output->config->driver.init_driver(err);
    if(err->err_type != NO_ERROR || map_to_output->driver_data == NULL)
    {
        goto map_to_output_init_error;
    }


    //need the tablet dimensions to calculate the aspect ratio
    map_to_output->config->driver.get_tablet_dimensions(
            map_to_output->driver_data,
            &map_to_output->tablet_width,
            &map_to_output->tablet_height,
            err);
    if(err->err_type != NO_ERROR)
    {
        //if get_tablet_dimensions failed the driver will have set *err
        goto map_to_output_init_error;
    }


    const double canvas_width = GTK_WIDGET(canvas)->allocation.width,
        canvas_height = GTK_WIDGET(canvas)->allocation.height;

    gboolean needs_new_page;
    OutputBox output_box = calculate_output_box(
            //blank canvas, therefore a new page
            //so the top left is the origin
            0, 0, 
            //canvas dimensions
            //TODO: should this be canvas allocation width and height?
            (int)canvas_width, (int)canvas_height,
            //tablet dimensions
            ((double)map_to_output->tablet_width) / ((double)map_to_output->tablet_height),
            &needs_new_page);

    if(needs_new_page)
    {
        //TODO: can't need a new page before we've even started
        //probably means our window is too small
        err->err_type = NEED_NEW_PAGE;
        err->err_msg = "Need a new page, try zooming out more";

        free_map_to_output(map_to_output);
        return NULL;
    }


    //call the tablet driver and map the tablet's output to our canvas region
    map_to_output->config->driver.map_to_output(
            map_to_output->driver_data,
            map_to_output,
            (unsigned int)output_box.top_left_x, 
            (unsigned int)output_box.top_left_y,
            (unsigned int)output_box.width,
            (unsigned int)output_box.height,
            err);
    if(err->err_type != NO_ERROR)
    {
        goto map_to_output_init_error;
    }
    else
    {
        //should have an active mapping after calling the driver
        assert(map_to_output->mapping_mode == MAPPING_ACTIVE);
    }

    //outline the mapped output box so the user can see it
    make_output_box_lines(map_to_output, output_box, canvas, err);
    if(err->err_type != NO_ERROR)
    {
        //reset the output mapping before returning
        MapToOutputError reset_err = no_error;
        map_to_output->config->driver.reset_map_to_output(map_to_output->driver_data, map_to_output, &reset_err);
        map_to_output_warn_if_error(&reset_err);
        
        goto map_to_output_init_error;
    }

    //run sanity checks before returning
    map_to_output_asserts(map_to_output);

    //set up globals
    G_LOCK(GLOBAL_MAP_TO_OUTPUT);
    GLOBAL_MAP_TO_OUTPUT = map_to_output;
    G_UNLOCK(GLOBAL_MAP_TO_OUTPUT);

    return map_to_output;

map_to_output_init_error:
    g_warn_if_reached();

    //if we're returning with an error we should have set an error message
    assert(err->err_type != NO_ERROR);

    if(map_to_output != NULL)
    {
        free(map_to_output);
    }
    return NULL;
}

//sanity checks (mostly null checks)
//should only be called after MapToOutput is initialized by map_to_output_init
void map_to_output_asserts(MapToOutput* map_to_output)
{
    assert(map_to_output != NULL);

    //see https://developer.gnome.org/glib/stable/glib-Warnings-and-Assertions.html
    //for glib warnings & assertions
    
    //do non-fatal checks before the assertions
    //if unsigned types are equal to their max, it's probably an error
    //(likely caused by an overflow)
    g_warn_if_fail(map_to_output->tablet_width == INT_MAX);
    g_warn_if_fail(map_to_output->tablet_height == INT_MAX);
    g_warn_if_fail(map_to_output->height == G_MAXUINT);
    g_warn_if_fail(map_to_output->width == G_MAXUINT);


    if(map_to_output->mapping_mode == NO_MAPPING)
    {
        assert(map_to_output->output_box == NULL);
    }
    else
    {
        assert(map_to_output->output_box != NULL);
    }

    assert(map_to_output->config != NULL);
    assert(map_to_output->canvas != NULL);
    assert(map_to_output->left_line != NULL);
    assert(map_to_output->right_line != NULL);
    assert(map_to_output->top_line != NULL);
    assert(map_to_output->bottom_line != NULL);

    assert(map_to_output->pixels_per_unit > 0);

}

void free_map_to_output(MapToOutput* map_to_output)
{
    //TODO
}
