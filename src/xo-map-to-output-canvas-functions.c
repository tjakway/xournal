#include "xo-map-to-output-canvas-functions.h"

#include <stdlib.h>
#include <assert.h>
#include <glib.h>

void map_to_output_get_canvas_drawing_area_dimensions(
        GnomeCanvas* canvas, Page* page, 
        double zoom,
        double* dx, double* dy, 
        double* dwidth, double* dheight, 
        gboolean* no_gap,
        MapToOutputError* err)
{
    //error checking
    if(canvas == NULL)
    {
        static const MapToOutputError arg_err = {
            .err_type = ARG_CHECK_FAILED,
            .err_msg = "error in "
                "map_to_output_get_canvas_drawing_area_dimensions:"
                " GnomeCanvas* is NULL"
        };
        *err = arg_err;
        goto get_canvas_drawing_area_dimensions_error;
    }
    if(page == NULL)
    {
        static const MapToOutputError arg_err = {
            .err_type = ARG_CHECK_FAILED,
            .err_msg = "error in "
                "map_to_output_get_canvas_drawing_area_dimensions:"
                " Page* is NULL"
        };
        *err = arg_err;
        goto get_canvas_drawing_area_dimensions_error;
    }
    if(zoom <= 0)
    {
        static const MapToOutputError arg_err = {
            .err_type = ARG_CHECK_FAILED,
            .err_msg = "error in "
                "map_to_output_get_canvas_drawing_area_dimensions:"
                " zoom <= 0"
        };
        *err = arg_err;
        goto get_canvas_drawing_area_dimensions_error;
    }

    //not worth writing error messages for, just fail
    assert(dx != NULL);
    assert(dy != NULL);
    assert(dwidth != NULL);
    assert(dheight != NULL);


    //*************
    

    //page width and height in pixels
    //the page is embedded in the canvas
    const double page_width_pixels = page->width * zoom,
          page_height_pixels = page->height;


    //we need to get accurate canvas dimensions and coordinates
    //this is more complicated than you would think
    
    //we need to call out to GDK to get the top left screen coordinate of our canvas...
    gint canvas_widget_top_left_x = -1, canvas_widget_top_left_y = -1;
    gdk_window_get_origin(gtk_widget_get_window(GTK_WIDGET(canvas)), &canvas_widget_top_left_x, &canvas_widget_top_left_y);

    //when dragging the window to full screen sometimes we get strange results
    if(canvas_widget_top_left_x < 0 || canvas_widget_top_left_y < 0)
    {
        fprintf(stderr, "Received nonsensical results from gdk_window_get_origin (x = %u, y = %u)," 
                " correcting to (0, 0)\n", canvas_widget_top_left_x, canvas_widget_top_left_y);
        canvas_widget_top_left_x = 0;
        canvas_widget_top_left_y = 0;
    }

    //the width of the canvas widget in pixels
    //!!__which may be larger than our page__!!
    const double canvas_width_pixels = (double)GTK_WIDGET(canvas)->allocation.width,
         canvas_height_pixels = (double)GTK_WIDGET(canvas)->allocation.height;

    assert(canvas_width_pixels > 0);
    assert(canvas_height_pixels > 0);

    //calculate the width difference between our page and the canvas widget (if any)
    const double width_gap = canvas_width_pixels - page_width_pixels;

    //optionally tell the caller whether the canvas is larger than our page
    if(no_gap != NULL)
    {
        //TODO: use margin of error
        if(width_gap == 0)
        {
            *no_gap = TRUE;
        }
        else
        {
            *no_gap = FALSE;
        }
    }

    //the top left of the page is the canvas' top left plus half the width gap
    //(the width gap is symmetric!)
    //the y value is the same
    *dx = canvas_widget_top_left_x + (width_gap / 2);
    *dy = canvas_widget_top_left_y;

    *dwidth = page_width_pixels;
    //the page is allowed to be taller than the canvas
    //(in which case scrolling will be turned on)
    //but the output box can't be taller than the canvas
    *dheight = canvas_height_pixels;
    return;


get_canvas_drawing_area_dimensions_error:
    g_warn_if_reached();
    
    //set parameters to -1
    *dx = -1;
    *dy = -1;
    *dwidth = -1;
    *dheight = -1;
    return;
}
