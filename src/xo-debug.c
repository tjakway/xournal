#include "xournal.h"
#include "xo-map-to-output-error.h"

#include <stdio.h>
#include <glib.h>

#ifdef DEBUG

//these functions aren't used anywhere but are a convenient way of getting
//info when stopped at breakpoints


void print_canvas_drawing_area_dimensions()
{
    //print info from globals
    if(canvas == NULL)
    {
        printf("%s: can't print because canvas == NULL\n", __func__);
        return;
    }
    if(ui.cur_page == NULL)
    {
        printf("%s: can't print because ui.cur_page == NULL\n", __func__);
        return;
    }

    Page* page = ui.cur_page;

    double zoom = ui.zoom;
    g_warn_if_fail(zoom > 0);

    gint x = -1, y = -1;
    double width = -1, height = -1;
    gboolean no_gap = FALSE;
    
    MapToOutputError err = no_error;

    map_to_output_get_canvas_drawing_area_dimensions(
            canvas, cur_page, zoom,
            &x, &y, &width, &height,
            &no_gap,
            &err);

    const char fname[] = "map_to_output_get_canvas_drawing_area_dimensions";
    if(err.err_type != NO_ERROR)
    {
        fprintf(stderr, 
                "MapToOutputError set by %s in %s:"
                " %s\n", fname, __func__, err.err_msg);
    }
    else
    {
        printf("%s results: x=%d, y=%d, width=%f, height=%f, no_gap=%s\n",
                x, y, width, height,
                no_gap ? "true" : "false");
    }
}


#endif
