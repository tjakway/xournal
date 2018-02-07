#include "xo-map-to-output-error.h"

#include <glib.h>

const MapToOutputError no_error = {
    .err_type = NO_ERROR,
    .err_msg = "Success"
};

const MapToOutputError bad_malloc = {
    .err_type = BAD_MALLOC,
    .err_msg = "malloc returned NULL"
};


void map_to_output_warn_if_error(MapToOutputError* err)
{
    if(err != NULL && err->err_type != NO_ERROR)
    {
        g_log(NULL, G_LOG_LEVEL_WARNING, "%s", err->err_msg);
    }
}
