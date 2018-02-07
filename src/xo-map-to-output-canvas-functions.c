#include "xo-map-to-output-canvas-functions.h"

#include <stdlib.h>
#include <assert.h>
#include <glib.h>

void map_to_output_get_canvas_drawing_area_dimensions(
        GnomeCanvas* canvas, Page* page, 
        double zoom,
        gint* dx, gint* dy, 
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

    if(canvas_widget_top_left_x < 0 || canvas_widget_top_left_y < 0)
    {
        static const MapToOutputError gdk_error = {
            .err_type = MAPTOOUTPUT_GDK_ERROR,
            .err_msg = "gdk_window_get_origin returned nonsensical results (x<0 or y<0)"
        };
        *err = gdk_error;
        goto get_canvas_drawing_area_dimensions_error;
    }

    //the width of the canvas widget in pixels
    //!!__which may be larger than our page__!!
    const double canvas_width_pixels = (double)GTK_WIDGET(canvas)->allocation.width,
         canvas_height_pixels = (double)GTK_WIDGET(canvas)->allocation.height;

    assert(canvas_width_pixels > 0);
    assert(canvas_height_pixels > 0);

    //the canvas shouldn't be taller than the page
    //TODO: use margin of error
    if(canvas_height_pixels != page_height_pixels)
    {
        static const MapToOutputError w_err = {
            .err_type = WIDGET_DIMENSION_ERROR,
            .err_msg = "unexpected result: canvas_height_pixels != page_height_pixels"
        };
        *err = w_err;
        goto get_canvas_drawing_area_dimensions_error;
    }

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

    //the top left of the page is the canvas' top left plus the width gap
    //the y value is the same
    *dx = canvas_widget_top_left_x + width_gap;
    *dy = canvas_widget_top_left_y;

    *dwidth = page_width_pixels;
    *dheight = page_height_pixels;
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
