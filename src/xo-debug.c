#ifdef DEBUG

//these functions aren't used anywhere but are a convenient way of getting
//info when stopped at breakpoints

#include "xournal.h"

#include <stdio.h>

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


}


#endif
