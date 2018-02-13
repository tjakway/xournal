#include "xo-map-to-output-callbacks.h"
#include "xo-map-to-output-error.h"

#include <stdio.h>

void map_to_output_on_window_changed(
        MapToOutput* map_to_output, 
        MapToOutputError* err)
{
    if(map_to_output != NULL && ERR_OK(err)
            && map_to_output->mapping_mode != NO_MAPPING)
    {
        map_to_output_do_mapping(map_to_output, FALSE, err);
    }
}


gboolean map_to_output_on_gdk_configure(
        GtkWidget* widget, GdkEventConfigure* configure_event, gpointer data)
{
#ifdef INPUT_DEBUG
    if(configure_event->type != GDK_CONFIGURE)
    {
        fprintf(stderr, "%s called: WARNING, event type is NOT GDK_CONFIGURE!\n", 
                __func__);
    }
    else
    {
        printf("%s called: send_event=%s, x=%d, y=%d, width=%d, height=%d\n", 
                __func__,
                configure_event->send_event ? "true" : "false", 
                configure_event->x, configure_event->y, configure_event->width,
                configure_event->height);
    }
#endif
    if(configure_event->type == GDK_CONFIGURE)
    {
        //call map_to_output_on_window_changed with the global map_to_output structure
        G_LOCK(GLOBAL_MAP_TO_OUTPUT);
        if(GLOBAL_MAP_TO_OUTPUT != NULL)
        {
            MapToOutputError* err = malloc(sizeof(MapToOutputError));
            *err = no_error;
            map_to_output_on_window_changed(GLOBAL_MAP_TO_OUTPUT, err);

            if(err->err_type != NO_ERROR)
            {
                fprintf(stderr, "Error in %s: "
                        "map_to_output_on_window_changed failed with message %s\n",
                        __func__, err->err_msg);
            }
            if(err != NULL)
            {
                free(err);
            }
        }
        G_UNLOCK(GLOBAL_MAP_TO_OUTPUT);
    }
}


void map_to_output_register_callbacks(GtkWidget* window)
{
    g_signal_connect(window, "configure_event", 
            G_CALLBACK(map_to_output_on_gdk_configure), NULL);
}
