#include "xo-map-to-output-callbacks.h"
#include "xo-map-to-output-error.h"

#include <stdio.h>

//1000 microseconds in a millisecond
#define MILLI_TO_MICROSECONDS 1000

volatile gint ATOMIC_NEED_REMAP_FLAG = FALSE;
#define NEED_REMAP g_atomic_int_get(&ATOMIC_NEED_REMAP_FLAG)
#define SET_NEED_REMAP_TRUE g_atomic_int_set(&ATOMIC_NEED_REMAP_FLAG, TRUE)
#define SET_NEED_REMAP_FALSE g_atomic_int_set(&ATOMIC_NEED_REMAP_FLAG, FALSE)

volatile gint SHUTDOWN_REMAP_THREAD_FLAG = FALSE;
#define SHUTDOWN_REMAP_THREAD g_atomic_int_get(&SHUTDOWN_REMAP_THREAD_FLAG)

static GThread* remap_thread = NULL;



//calls the driver indirectly
static void map_to_output_on_window_changed(
        MapToOutput* map_to_output, 
        MapToOutputError* err)
{
    if(map_to_output != NULL && ERR_OK(err)
            && map_to_output->mapping_mode != NO_MAPPING)
    {
        map_to_output_do_mapping(map_to_output, FALSE, err);
    }
}


void shutdown_remap_thread()
{
    //tell the thread to shutdown 
    g_atomic_int_set(&SHUTDOWN_REMAP_THREAD_FLAG, TRUE);

    //and wait for it to do so
    if(remap_thread != NULL)
    {
        g_thread_join(remap_thread);
        g_thread_unref(remap_thread);
    }
}

//window events only call the driver indirectly: by setting a flag that gets
//read by this thread that then calls the driver
//this is to prevent a flood of driver calls because sometimes GTK
//will send a TON of GDK_CONFIGURE events
gpointer remap_thread_work(gpointer __ignored)
{
    do
    {
        //glib function to sleep for the passed number of microseconds
        usleep(REMAP_THREAD_INTERVAL * MILLI_TO_MICROSECONDS);

        //perform the remap if we have to
        if(NEED_REMAP)
        {
            //call map_to_output_on_window_changed with the global map_to_output structure
            G_LOCK(GLOBAL_MAP_TO_OUTPUT);
            if(GLOBAL_MAP_TO_OUTPUT != NULL)
            {
                MapToOutputError* err = malloc(sizeof(MapToOutputError));
                if(err != NULL)
                {
                    *err = no_error;
                    map_to_output_on_window_changed(GLOBAL_MAP_TO_OUTPUT, err);

                    if(err->err_type != NO_ERROR)
                    {
                        fprintf(stderr, "Error in %s: "
                                "map_to_output_on_window_changed failed with message %s\n",
                                __func__, err->err_msg);
                    }

                    free(err);
                }
                else
                {
                    fprintf(stderr, "could not allocate a MapToOutputError struct in %s in %s at line %d",
                            __func__, __FILE__, __LINE__);
                }
            }
            G_UNLOCK(GLOBAL_MAP_TO_OUTPUT);

            SET_NEED_REMAP_TRUE;
        }
    } while(!SHUTDOWN_REMAP_THREAD);
    return NULL;
}

void launch_remap_thread(MapToOutputError* err)
{
    g_atomic_int_set(&SHUTDOWN_REMAP_THREAD_FLAG, FALSE);

    GError* gerr_ptr = NULL;
    GThread* new_remap_thread = g_thread_try_new(
            REMAP_THREAD_NAME,
            &remap_thread_work, 
            NULL, &err);

    //check that launch was successful, warn the user otherwise
    if(new_remap_thread == NULL)
    {
        XO_LOG_GERROR("launching the remap thread");
        if(err != NULL)
        {
            *err = (MapToOutputError){
                .err_type = GLIB_THREAD_ERROR,
                .err_msg = "Could not launch the remap thread because "
                    "g_thread_try_new returned NULL (likely due to "
                    "resource constraints)"
            };
        }
    }
    else
    {
        remap_thread = new_remap_thread;
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
    //flag the remap thread
    if(configure_event->type == GDK_CONFIGURE)
    {
        SET_NEED_REMAP_TRUE;
    }

    return FALSE;
}


void map_to_output_register_callbacks(GtkWidget* window, MapToOutputError* err)
{
    if(ERR_OK(err))
    {
        g_signal_connect(window, "configure_event", 
                G_CALLBACK(map_to_output_on_gdk_configure), NULL);

        launch_remap_thread(err);
    }
}
