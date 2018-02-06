#include "xo-wacom-driver.h"
#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"

#include <glib.h>
#include <stdlib.h>

#define ONLY_WHITESPACE_RGX "/\\A\\s*\\z/"
#define MATCH_STYLUS_RGX "[" //TODO

typedef struct ParsingRegexes {
    GRegex* only_whitespace;
    GRegex* match_stylus;
} ParsingRegexes;


static void free_parsing_regexes(ParsingRegexes*);

static ParsingRegexes* init_parsing_regexes()
{
    ParsingRegexes* p_rgx = malloc(sizeof(ParsingRegexes));
    if(p_rgx == NULL)
    {
        return NULL;
    }
    else
    {
        *p_rgx = (ParsingRegexes){NULL, NULL};
    }

    const GRegexCompileFlags compile_flags = 
        //match across newlines and make . match newlines
        G_REGEX_DOTALL | 
        G_REGEX_MULTILINE |
        G_REGEX_CASELESS;


    GError* err = NULL;
    p_rgx->only_whitespace = 
        g_regex_new(
            ONLY_WHITESPACE_RGX,
            compile_flags, 0,
            &err);
    if(p_rgx->only_whitespace == NULL
            || err != NULL)
    {
        //it's safe to call free_parsing_regexes with NULL fields,
        //it will check appropriately
        free_parsing_regexes(p_rgx);
        return NULL;
    }


    p_rgx->match_stylus = 
        g_regex_new(
                MATCH_STYLUS_RGX,
                compile_flags, 0,
                &err);

    if(p_rgx->match_stylus == NULL
            || err != NULL)
    {
        free_parsing_regexes(p_rgx);
        return NULL;
    }


    return p_rgx;
}

static void free_parsing_regexes(ParsingRegexes* p_rgx)
{
    if(p_rgx != NULL)
    {
        if(p_rgx->only_whitespace != NULL)
        {
            g_regex_unref(p_rgx->only_whitespace);
        }

        if(p_rgx->match_stylus != NULL)
        {
            g_regex_unref(p_rgx->match_stylus);
        }

        free(p_rgx);
    }

}


void* init_wacom_driver(MapToOutputError* err)
{
    g_warn_if_fail(err != NULL);

    WacomTabletData* wacom_data = malloc(sizeof(WacomTabletData));
    if(wacom_data == NULL)
    {
        if(err != NULL)
        {
            *err = bad_malloc;
        }
        return NULL;
    }
    else
    {
        *wacom_data = (WacomTabletData){0};
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


char* wacom_parse_device_name(const char* xsetwacom_stdout)
{
    g_warn_if_fail(xsetwacom_stdout != NULL);

}


/**
 * TODO: fill in fields
 */
const TabletDriver wacom_driver = {
    .init_driver = &init_wacom_driver
};
