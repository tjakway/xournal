#include "xournal.h"
#include "xo-map-to-output-error.h"
#include "xo-map-to-output-canvas-functions.h"

#include <stdio.h>
#include <glib.h>
#include <float.h> //for DBL_MAX

#include <libgnomecanvas/libgnomecanvas.h>

#ifdef DEBUG

//these functions aren't used anywhere but are a convenient way of getting
//info when stopped at breakpoints


//return if any necessary globals are null
#define DEBUG_PRINT_CHECK_GLOBALS() do { if(canvas == NULL) \
    { \
        printf("%s: can't print because canvas == NULL\n", __func__); \
        return; \
    } \
    if(ui.cur_page == NULL) \
    { \
        printf("%s: can't print because ui.cur_page == NULL\n", __func__); \
        return; \
    } } while(0);

void print_canvas_drawing_area_dimensions()
{
    DEBUG_PRINT_CHECK_GLOBALS();

    Page* page = ui.cur_page;

    double zoom = ui.zoom;
    g_warn_if_fail(zoom > 0);

    gint x = -1, y = -1;
    double width = -1, height = -1;
    gboolean no_gap = FALSE;
    
    MapToOutputError err = no_error;

    map_to_output_get_canvas_drawing_area_dimensions(
            canvas, page, zoom,
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
                fname, 
                x, y, width, height,
                no_gap ? "true" : "false");
    }
}

void print_canvas_w2c_affine_matrix()
{
    DEBUG_PRINT_CHECK_GLOBALS();

    const int affine_len = 6;
    double affine[affine_len];

    for(int i = 0; i < affine_len; i++)
    {
        affine[i] = DBL_MAX;
    }

    gnome_canvas_w2c_affine(canvas, affine);

    gboolean affine_not_set = FALSE;
    for(int i = 0; i < affine_len; i++)
    {
        if(affine[i] == DBL_MAX)
        {
            affine_not_set = TRUE;
        }
    }

    if(affine_not_set)
    {
        fprintf(stderr, "gnome_canvas_w2c_affine didn't set the affine matrix in %s!\n", __func__);
    }
    else
    {
        printf("w2c affine matrix: { %f, %f, %f, %f, %f, %f }\n",
                affine[0], affine[1], affine[2], affine[3], affine[4], affine[5]);
    }
}

void print_canvas_w2c(double x, double y)
{
    DEBUG_PRINT_CHECK_GLOBALS();

    int ret_x = -1, ret_y = -1;

    gnome_canvas_w2c_d(canvas, x, y, &ret_x, &ret_y);

    if(ret_x < 0 || ret_y < 0)
    {
        fprintf(stderr, "WARNING: gnome_canvas_w2c_d returned x < 0 || y < 0 in %s\n", __func__);
    }

    printf("print_canvas_w2c(%f, %f): (%d, %d)\n", x, y, ret_x, ret_y);
}

#endif
