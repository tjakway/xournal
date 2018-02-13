#pragma once

#include <glib.h>
#include <gtk/gtk.h>

#include "xo-map-to-output.h"

//the callbacks need access to our data structures but we can't pass
//them so globals are the only option
extern MapToOutput* GLOBAL_MAP_TO_OUTPUT;
G_LOCK_EXTERN(GLOBAL_MAP_TO_OUTPUT);


gboolean map_to_output_on_gdk_configure(
        GtkWidget*, GdkEventConfigure*, gpointer);

void map_to_output_register_callbacks(GtkWidget*, MapToOutputError*);

//sets a flag to shutdown the remap thread
void shutdown_remap_thread();

//how often the remap thread should run, in milliseconds
#define REMAP_THREAD_INTERVAL 100
#define REMAP_THREAD_NAME "remap_thread"
