#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"

#include <glib.h>
#include <stdlib.h>


#define WACOM_DRIVER "xsetwacom"

//TODO: delete if we don't use g_spawn_sync()
static const GSpawnFlags COMMON_FLAGS = G_SPAWN_SEARCH_PATH;


static const MapToOutputError driver_program_not_found = {
    .err_type = DRIVER_PROGRAM_NOT_FOUND,
    .err_msg = "Could not find tablet driver named " WACOM_DRIVER
};


typedef struct WacomTabletData {
    const char* device_name;

} WacomTabletData;

static void* init_wacom_driver(MapToOutputError* err)
{
    g_warn_if_fail(err == NULL);

    WacomTabletData* wacom_data = malloc(sizeof(WacomTabletData));
    if(wacom_data == NULL)
    {
        if(err != NULL)
        {
            *err = bad_malloc;
        }
        return NULL;
    }

    //run xsetwacom and get device names
    //TODO: double check how I'm supposed to get stdout back...
    gchar* stdout_sink = NULL;
    gint exit_code = -1;
    GError* gerr_ptr = NULL;
    const char* cmd = WACOM_DRIVER " --list devices";
    gboolean res = g_spawn_command_line_sync(cmd,
            &stdout_sink,
            NULL,
            &exit_code,
            &gerr_ptr);


    if(!res)
    {
        //check specifically if we couldn't find the driver program
        if(gerr_ptr != NULL 
                && gerr_ptr->code == G_FILE_ERROR_EXIST
                && err != NULL)
        {
            *err = driver_program_not_found;
        }
        //otherwise print the warning message
        else
        {
            char* msg;
            if(gerr_ptr != NULL)
            {
                msg = gerr_ptr->message;
            }
            else
            {
                msg = "gerr_ptr == NULL, no glib error string available";
            }
            g_log(NULL, G_LOG_LEVEL_WARNING, 
                    "error in %s during call to tablet driver: `%s`, "
                    "glib reports error as: %s\n",
                    __func__, cmd, msg);

        }

        goto init_wacom_driver_error;
    }
    else
    {
        //process the driver output
        //stdout_sink
    }

init_wacom_driver_error:
    free(wacom_data);

    if(stdout_sink != NULL)
    {
        free(stdout_sink);
    }
    return NULL;
}


/**
 * TODO: fill in fields
 */
const TabletDriver wacom_driver = {
    .init_driver = &init_wacom_driver
};
