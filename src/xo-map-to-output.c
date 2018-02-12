#include "xournal.h"
#include "xo-map-to-output.h"
#include "xo-map-to-output-callbacks.h"
#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"
#include "xo-map-to-output-canvas-functions.h"
#include "xo-callbacks.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**********************/
/*******GLOBALS********/
/**********************/
//allocate the globals forward-declared in xo-map-to-output-callbacks.h
MapToOutput* GLOBAL_MAP_TO_OUTPUT;
G_LOCK_DEFINE(GLOBAL_MAP_TO_OUTPUT);
/**********************/

#define NUM_BOX_COORDS 8

OutputBox calculate_initial_output_box(
        MapToOutput* map_to_output, //only need for the tablet aspect ratio
        GnomeCanvas* canvas, Page* page, double zoom,
        MapToOutputError* err);

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

double map_to_output_get_tablet_aspect_ratio(MapToOutput* map_to_output)
{
    g_warn_if_fail(map_to_output != NULL);
    g_warn_if_fail(map_to_output->tablet_width > 0);
    g_warn_if_fail(map_to_output->tablet_height > 0);

    return ((double)map_to_output->tablet_width) / ((double)map_to_output->tablet_height);
}

gboolean output_box_is_visible(
        GnomeCanvas* canvas, Page* page, double zoom, OutputBox box,
        MapToOutputError* err)
{
    //get viewable area dimensions
    double viewable_x = -1, viewable_y = -1,
           viewable_width = -1, viewable_height = -1;
    gboolean _no_gap;

    map_to_output_get_canvas_drawing_area_dimensions(
            canvas, page, zoom,
            &viewable_x, &viewable_y,
            &viewable_width, &viewable_height,
            &_no_gap, err);

    //2 doubles per coord X 4 coords
    double box_coords[NUM_BOX_COORDS] = {
        //top left
        box.top_left_x, box.top_left_y,

        //top right
        box.top_left_x + box.width, box.top_left_y,

        //bottom left
        box.top_left_x, box.top_left_y + box.height,

        //bottom right
        box.top_left_x + box.width, box.top_left_y + box.height
    };

    const int num_visible_points = 4;
    gboolean visible_points[num_visible_points];
    for(int i = 0; i < num_visible_points; i++)
    {
        visible_points[i] = FALSE;
    }

    for(int i = 0; i < NUM_BOX_COORDS; i +=2)
    {
        double window_x = -1, window_y = -1;
        gnome_canvas_world_to_window(canvas, 
            box_coords[i],
            box_coords[i + 1],
            &window_x, &window_y);

        
        const gboolean x_within_range = 
            window_x >= viewable_x &&
            window_x <= (viewable_x + viewable_width);

        const gboolean y_within_range =
            window_y >= viewable_y &&
            window_y <= (viewable_y + viewable_height);

        visible_points[i / 2] = x_within_range && y_within_range;
    }

    //log any nonvisible points
    for(int i = 0; i < num_visible_points; i++)
    {
        if(!visible_points[i])
        {
            g_debug("output box coord %d not visible", i);
        }
    }

    //return true if all points are visible
    return visible_points[0] && visible_points[1] &&
        visible_points[2] && visible_points[3];
}

void output_box_to_array(OutputBox output_box, double points[8])
{
    const double cpy_points[8] = { output_box.top_left_x, output_box.top_left_y, 
      output_box.top_left_x + output_box.width, output_box.top_left_y,
      output_box.top_left_x, output_box.top_left_y  + output_box.height,
      output_box.top_left_x + output_box.width, output_box.top_left_y  + output_box.height
    };

    memcpy(points, cpy_points, sizeof(double)*8);
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

static void set_line_points(GnomeCanvasItem* item,
        double points[2], MapToOutputError* err)
{
    GnomeCanvasPoints* canvas_points = gnome_canvas_points_new(2);
    if(canvas_points == NULL)
    {
        *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
    }
    else
    {
        canvas_points->coords[0] = points[0];
        canvas_points->coords[1] = points[1];

        gnome_canvas_item_set(item, "points", points, NULL);
    }
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

    if(!ERR_OK(err))
    { return NULL; }

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

    map_to_output->config->driver.get_tablet_dimensions(map_to_output->driver_data,
            &map_to_output->tablet_width,
            &map_to_output->tablet_height,
            err);
    if(!ERR_OK(err))
    {
        goto map_to_output_init_error;
    }



    map_to_output_do_mapping(map_to_output, TRUE, err);

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

OutputBox calculate_initial_output_box(
        MapToOutput* map_to_output, //only need for the tablet aspect ratio
        GnomeCanvas* canvas, Page* page, double zoom,
        MapToOutputError* err)
{
    //calculate the initial output box's width
    //by setting it equal to the width in world coordinates that 
    //corresponds to the width of the entire canvas in pixels
    gboolean no_gap;
    double canvas_viewable_x, canvas_viewable_y,
           canvas_viewable_width, canvas_viewable_height;
    map_to_output_get_canvas_drawing_area_dimensions(
            canvas, page, zoom,
            &canvas_viewable_x,
            &canvas_viewable_y,
            &canvas_viewable_width,
            &canvas_viewable_height,
            &no_gap,
            err);

    //get the world coordinates for the top left and top right of the canvas'
    //viewable drawing area
    double wc_left_x = -1, wc_left_y = -1,
           wc_right_x = -1, wc_right_y = -1;

    gnome_canvas_window_to_world(canvas,
            canvas_viewable_x, canvas_viewable_y,
            &wc_left_x, &wc_left_y);
    gnome_canvas_window_to_world(canvas,
            canvas_viewable_x + canvas_viewable_width,
            canvas_viewable_y,
            &wc_right_x, &wc_right_y);


    const int initial_width = wc_right_x - wc_left_x;
    const double initial_width_d = (double)initial_width;
    const int initial_height = 
                 (int)(initial_width_d
                         / map_to_output_get_tablet_aspect_ratio(map_to_output));

    const OutputBox initial_output_box = {
        .top_left_x = wc_left_x,
        .top_left_y = wc_left_y,
        .width = initial_width,
        .height = initial_height
    };

    return initial_output_box;
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
    g_warn_if_fail(map_to_output->tablet_width != INT_MAX);
    g_warn_if_fail(map_to_output->tablet_height != INT_MAX);
    g_warn_if_fail(map_to_output->height != G_MAXUINT);
    g_warn_if_fail(map_to_output->width != G_MAXUINT);


    if(map_to_output->mapping_mode == NO_MAPPING)
    {
        assert(map_to_output->output_box == NULL);
    }
    else
    {
        assert(map_to_output->output_box != NULL);
    }

    assert(map_to_output->config != NULL);
    assert(map_to_output->left_line != NULL);
    assert(map_to_output->right_line != NULL);
    assert(map_to_output->top_line != NULL);
    assert(map_to_output->bottom_line != NULL);

    assert(map_to_output->pixels_per_unit > 0);

}

OutputBox shift_output_box_down(OutputBox box)
{
    box.top_left_y += box.height;
    return box;
}

gboolean output_box_within_page(OutputBox box, Page* page)
{
    double points[8];
    output_box_to_array(box, points);

    //check x
    for(int i = 0; i < 8; i +=2)
    {
        if(points[i] < page->hoffset 
                || points[i] > page->hoffset + page->width
                //check y
                || points[i + 1] < page->voffset 
                || points[i + 1] < page->voffset + page->height)
        {
            return FALSE;
        }
    }
    return TRUE;
}

enum ShiftDownResult predict_shift_down_changes(
        GnomeCanvas* canvas, Page* page, double zoom,
        MapToOutput* map_to_output, 
        MapToOutputError* err)
{
    //don't do anything if we've already encountered an error
    if(err->err_type != NO_ERROR)
    {
        return ERROR;
    }

    enum ShiftDownResult res = NO_SCREEN_CHANGE;
    if(map_to_output->output_box == NULL)
    {
        *err = (MapToOutputError){
            .err_type = NO_MAPPING_ERROR,
            .err_msg = "map_to_output_shift_down was called when a mapping was not active"
        };
        return ERROR;
    }
    else
    {
        //see if the shifted output box would be visible
        OutputBox shifted_output_box = shift_output_box_down(*map_to_output->output_box);

        //check if the shifted output box runs off the page
        if(output_box_within_page(shifted_output_box, page))
        {
            res = NEW_PAGE;
        }
        else
        {
            //if not, make sure it's still visible
            gboolean visible = output_box_is_visible(canvas, ui.cur_page,
                    ui.zoom, shifted_output_box, err);

            if(err->err_type != NO_ERROR)
            {
                return ERROR;
            }

            //tell the caller whether or not the screen will need to be moved
            if(!visible)
            {
                res = MOVE_SCREEN;
            }
        }
    }

    return res;
}

void map_to_output_coords_from_output_box(MapToOutput* map_to_output,
        OutputBox box, MapToOutputError* err)
{
    //get screen coords from the output box
    double window_top_left_x = -1, window_top_left_y = -1;

    gnome_canvas_world_to_window(canvas,
            box.top_left_x, box.top_left_y,
            &window_top_left_x, &window_top_left_y);

    double window_top_right_x = -1, window_top_right_y = -1;
    gnome_canvas_world_to_window(canvas,
            box.top_left_x + box.width, box.top_left_y,
            &window_top_right_x, &window_top_right_y);

    double window_bottom_left_x = -1, window_bottom_left_y = -1;
    gnome_canvas_world_to_window(canvas,
            box.top_left_x, box.top_left_y + box.height,
            &window_bottom_left_x, &window_bottom_left_y);
    
    const unsigned int map_x = (unsigned int)window_top_left_x, 
                       map_y = (unsigned int)window_top_left_y,
                       map_width = (unsigned int)(window_top_right_x - window_top_left_x),
                       map_height = (unsigned int)(window_bottom_left_y - window_top_left_y);

    map_to_output->config->driver.map_to_output(
            map_to_output->driver_data,
            map_to_output,
            map_x, map_y, map_width, map_height, err);
}

void update_lines_from_output_box(
        MapToOutput* map_to_output,
        OutputBox box, MapToOutputError* err)
{
    double top[2], right[2], bottom[2], left[2];

    //get line points
    output_box_to_lines(box, top, right, bottom, left);

    //update the points for the canvas line items
    if(err->err_type == NO_ERROR) {
        set_line_points(map_to_output->top_line, top, err);
    }
    if(err->err_type == NO_ERROR) {
        set_line_points(map_to_output->right_line, right, err);
    }
    if(err->err_type == NO_ERROR) {
        set_line_points(map_to_output->bottom_line, bottom, err);
    }
    if(err->err_type == NO_ERROR) {
        set_line_points(map_to_output->left_line, left, err);
    }
}

/**
 * scroll down by 1 unit
 */
static void scroll_down_1_unit(GnomeCanvas* canvas)
{
    int hoffset = -1, voffset = -1;
    gnome_canvas_get_scroll_offsets(canvas, &hoffset, &voffset);
    gnome_canvas_scroll_to(canvas, hoffset, voffset + 1);
}

/**
 * scroll to the output box or do nothing if it's already visible
 */
static void scroll_to_output_box(
        MapToOutput* map_to_output, 
        GnomeCanvas* canvas,
        Page* page,
        double zoom,
        OutputBox output_box,
        MapToOutputError* err)
{
    //programmatically scroll down until the outputbox is visible
    while(ERR_OK(err) && !output_box_is_visible(canvas, page, zoom, output_box, err))
    {
        scroll_down_1_unit(canvas);
        //TODO: add checks that we didn't scroll too far...
    }
}

void map_to_output_shift_down(
        MapToOutput* map_to_output, 
        GnomeCanvas* canvas,
        Page* page,
        double zoom,
        OutputBox output_box,
        MapToOutputError* err)
{
    if(!ERR_OK(err))
    { return; }

    OutputBox shifted_output_box = shift_output_box_down(*map_to_output->output_box);

    enum ShiftDownResult shift_result = 
        predict_shift_down_changes(canvas, page, zoom, map_to_output, err);
    if(!ERR_OK(err))
    { return; }

    //we'll need a new page so create one programmatically
    if(shift_result == NEW_PAGE)
    {
        on_journalNewPageAfter_activate(NULL, NULL);
    }


    //save the current output box in case there's an error
    OutputBox old_output_box = *map_to_output->output_box;
    *map_to_output->output_box = shifted_output_box;

    //refresh the mapping
    map_to_output_do_mapping(map_to_output, FALSE, err);
    if(!ERR_OK(err))
    {
        //reset the output box before returning
        *map_to_output->output_box = old_output_box;
    }
}

/**
 * allow_new: whether or not to create a new mapping
 * (if false and mapping_mode == NO_MAPPING then nothing will be done)
 */
void map_to_output_do_mapping(
        MapToOutput* map_to_output,
        gboolean allow_new,
        MapToOutputError* err)
{
    g_warn_if_fail(map_to_output != NULL);
    g_warn_if_fail(err != NULL);

    if(!ERR_OK(err))
    {
        return;
    }

    if(!allow_new && map_to_output->mapping_mode == NO_MAPPING)
    {
        return;
    }
    else if(allow_new && map_to_output->mapping_mode == NO_MAPPING)
    {
        //create a new mapping
        

        //calculate the initial output box
        const OutputBox initial_output_box = calculate_initial_output_box(
                map_to_output, canvas, ui.cur_page, ui.zoom, err);
        if(!ERR_OK(err))
            { return; } 

        //allocate space for the new output box
        if(map_to_output->output_box == NULL)
        {
            map_to_output->output_box = malloc(sizeof(OutputBox));
            if(map_to_output->output_box == NULL)
            {
                *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
            }
            else
            {
                *map_to_output->output_box = initial_output_box;
            }
        }
        if(!ERR_OK(err))
            { return; } 

        //outline it on the canvas
        make_output_box_lines(map_to_output, initial_output_box, canvas, err);
        if(!ERR_OK(err))
            { return; } 

        //make sure the output box is visible
        scroll_to_output_box(map_to_output, canvas, 
                ui.cur_page, ui.zoom, *map_to_output->output_box, err);
        if(!ERR_OK(err))
            { return; } 

        //call the driver
        map_to_output_coords_from_output_box(map_to_output, *map_to_output->output_box, err);
    }
    else if(map_to_output->mapping_mode != NO_MAPPING)
    {
        //it's an existing mapping


        if(map_to_output->output_box == NULL)
        {
            *err = (MapToOutputError){
                .err_type = NO_OUTPUT_BOX,
                .err_msg = "map_to_output->output_box == NULL even though "
                    "mapping_mode != NO_MAPPING (line " LINE_STR " of " __FILE__ ")"
            };
            return;
        }
        else
        {
            //make sure the canvas is drawing the box in the correct position
            update_lines_from_output_box(map_to_output, *map_to_output->output_box, err);
            if(!ERR_OK(err))
                { return; } 

            //make sure the output box is visible
            scroll_to_output_box(map_to_output, canvas, 
                    ui.cur_page, ui.zoom, *map_to_output->output_box, err);
            if(!ERR_OK(err))
                { return; } 

            //call the driver
            map_to_output_coords_from_output_box(map_to_output, *map_to_output->output_box, err);
            if(!ERR_OK(err))
                { return; } 

        }
    }
}



void free_map_to_output(MapToOutput* map_to_output)
{
    //TODO
}
