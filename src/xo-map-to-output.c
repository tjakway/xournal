#include "xo-map-to-output.h"

#include <stdlib.h>

//TODO: fill in default config
static MapToOutputConfig default_config;

static MapToOutputError no_error {
    .err_type = NO_ERROR,
    .err_msg = "Success"
};

static MapToOutputError bad_malloc {
    .err_type = BAD_MALLOC,
    .err_msg = "malloc returned NULL"
};


static void get_tablet_dimensions(
        unsigned int* width,
        unsigned int* height,
        MapToOutputError* err)
{

}


MapToOutput* map_to_output_init(
        MapToOutputConfig* config_param, 
        GnomeCanvas* canvas,
        guint width,
        guint height,
        MapToOutputError* err)
{
    MapToOutputConfig* config;
    if(config_param == NULL)
    {
        config = &default_config;
    }
    else
    {
        config = config_param;
    }

    MapToOutput* map_to_output = malloc(sizeof(MapToOutput));
    if(map_to_output == NULL)
    {
        *err = bad_malloc;
        return NULL;
    }

}
