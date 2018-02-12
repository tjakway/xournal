#pragma once

#include <glib.h>
#include <gtk/gtk.h>

#include "xo-map-to-output.h"

//the callbacks need access to our data structures but we can't pass
//them so globals are the only option
extern MapToOutput* GLOBAL_MAP_TO_OUTPUT;
G_LOCK_EXTERN(GLOBAL_MAP_TO_OUTPUT);


/**
 * TODO: this is necessary because we're mapping **screen** pixels to the canvas so we need
 * to resize it such that the same window is shown
 */
void map_to_output_on_window_changed(
        MapToOutput*, 
        MapToOutputError*);


gboolean map_to_output_on_gdk_configure(
        GtkWidget*, GdkEventConfigure*, gpointer);

void map_to_output_register_callbacks(GtkWidget*);
