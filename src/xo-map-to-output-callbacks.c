#include "xo-map-to-output-callbacks.h"

#include <stdio.h>

void map_to_output_on_window_changed(
        MapToOutput* map_to_output, 
        MapToOutputError* err)
{
    if(map_to_output->mapping_mode != NO_MAPPING)
    {
        //TODO
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
}


void map_to_output_register_callbacks(GtkWidget* window)
{
    g_signal_connect(window, "configure_event", 
            G_CALLBACK(map_to_output_on_gdk_configure), NULL);
}
