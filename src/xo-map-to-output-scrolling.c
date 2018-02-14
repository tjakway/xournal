#pragma once

#include "xo-map-to-output.h"
#include "xo-map-to-output-error.h"
#include "xo-map-to-output-scrolling.h"


#include <assert.h>
#include <limits.h>

#include <libgnomecanvas/libgnomecanvas.h>

#define DEFAULT_SCROLL_PLAN_SIZE 500
#define DEFAULT_SCROLL_PLAN_INCREMENT 100

typedef enum ScrollAction {
    SCROLL_UP=2,
    SCROLL_DOWN=3,
    SCROLL_LEFT=4,
    SCROLL_RIGHT=5
} ScrollAction;

static ScrollAction* make_scroll_plan(GnomeCanvas* canvas, 
        OutputBox* box,
        int* length, 
        MapToOutputError* err)
{
    //error checks
    assert(err != NULL); assert(canvas != NULL); 
    assert(length != NULL); assert(box != NULL);

    g_warn_if_fail(err != NULL);

    if(box == NULL)
    {
        *err = (MapToOutputError){
            .err_type = NO_OUTPUT_BOX,
            .err_msg = "No output box in make_scroll_plan in "
                __FILE__ " at line " LINE_STR
        };
        return NULL;
    }

    //array length we've allocated and how much we've actually used
    int scroll_plan_alloced_length = DEFAULT_SCROLL_PLAN_SIZE,
        scroll_plan_length = 0;
    ScrollAction* scroll_plan = NULL;

    scroll_plan = malloc(sizeof(ScrollAction) * scroll_plan_alloced_length);
    if(scroll_plan == NULL)
    {
        *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
    }

    gboolean done_scrolling = FALSE;
    while(!done_scrolling)
    {

        //allocate more memory if needed
        if((scroll_plan_length - 1) == scroll_plan_alloced_length)
        {
            //sanity checks
            if((scroll_plan_length - 1) == INT_MAX)
            {
                *err = (MapToOutputError){
                    .err_type = SCROLL_PLAN_ERROR,
                    .err_msg = "scroll_plan_length - 1 == INT_MAX, "
                        "something is definitely wrong"
                };
                free(scroll_plan);
                return NULL;
            }
            else if(scroll_plan_alloced_length > (INT_MAX - DEFAULT_SCROLL_PLAN_INCREMENT))
            {
                *err = (MapToOutputError){
                    .err_type = SCROLL_PLAN_ERROR,
                    .err_msg = "scroll_plan_alloced_length > (INT_MAX - DEFAULT_SCROLL_PLAN_INCREMENT)"
                        ", reallocation would overflow"
                };
                free(scroll_plan);
                return NULL;
            }
            else if(scroll_plan_alloced_length < DEFAULT_SCROLL_PLAN_SIZE)
            {
                *err = (MapToOutputError){
                    .err_type = SCROLL_PLAN_ERROR,
                    .err_msg = "scroll_plan_alloced_length < DEFAULT_SCROLL_PLAN_SIZE"
                        ", an overflow has occurred"
                };
                free(scroll_plan);
                return NULL;
            }
            else
            {
                //allocate more memory and keep going
                scroll_plan_alloced_length += DEFAULT_SCROLL_PLAN_INCREMENT;
                scroll_plan = realloc(scroll_plan, scroll_plan_alloced_length);

                if(scroll_plan == NULL)
                {
                    *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
                }
            }
        }
    }
    

}

/**
 * scroll down by 1 unit
 */
void scroll_down_1_unit(GnomeCanvas* canvas)
{
    //TODO: FIX SIGSEGV
    int hoffset = -1, voffset = -1;
    gnome_canvas_get_scroll_offsets(canvas, &hoffset, &voffset);
    gnome_canvas_scroll_to(canvas, hoffset, voffset + 1);
    
#ifdef INPUT_DEBUG
    printf("%s: hoffset=%d, voffset=%d\n", __func__, hoffset, voffset);
#endif
}

/**
 * scroll to the output box or do nothing if it's already visible
 */
void scroll_to_output_box(
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
