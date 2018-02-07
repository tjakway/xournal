#include "xo-wacom-driver.h"
#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"

#include <glib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define ONLY_WHITESPACE_RGX "/\\A\\s*\\z/"
#define MATCH_STYLUS_RGX "[" //TODO


#define XO_LOG_GERROR(cmd) do { char* msg; \
            if(gerr_ptr != NULL) \
            { \
                msg = gerr_ptr->message; \
            } \
            else \
            { \
                msg = "gerr_ptr == NULL, no glib error string available"; \
            } \
            g_log(NULL, G_LOG_LEVEL_WARNING, \
                    "error in %s during call to tablet driver: `%s`, " \
                    "glib reports error as: %s\n", \
                    __func__, cmd, msg); \
            } while(0);


static void free_parsing_regexes(struct WacomParsingRegexes*);

static struct WacomParsingRegexes* init_parsing_regexes()
{
    struct WacomParsingRegexes* p_rgx = malloc(sizeof(struct WacomParsingRegexes));
    if(p_rgx == NULL)
    {
        return NULL;
    }
    else
    {
        *p_rgx = (struct WacomParsingRegexes){ 0 };
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

static void free_parsing_regexes(struct WacomParsingRegexes* p_rgx)
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

static gboolean is_empty_string(struct WacomParsingRegexes* p_rgx, const char* str)
{
    return g_regex_match(p_rgx->only_whitespace,
            str,0, NULL);
}

char* wacom_parse_device_name(void* v, const char* str, MapToOutputError* err)
{
    g_warn_if_fail(v != NULL); g_warn_if_fail(str != NULL);
    g_warn_if_fail(err != NULL);

    WacomTabletData* data = (WacomTabletData*)v;

    //can't have a device name if it's empty
    if(is_empty_string(data->p_rgx, str))
    {
        return NULL;
    }
    else
    {
        char* ret = NULL;
        GMatchInfo* match = NULL;

        gboolean matched = g_regex_match_all(data->p_rgx->match_stylus, str, 0, &match);

        if(matched)
        {
            //TODO: don't forget to free with g_strfreev()
            gchar** all_matches = g_match_info_fetch_all(match);

            if(all_matches != NULL)
            {
                //the gchar** returned by g_match_info_fetch_all is null terminated
                
                //count how many matches there are
                int num_matches = 0;
                char* this_match = NULL;
                do {
                    this_match = all_matches[num_matches];

                    if(this_match != NULL && 
                            //sanity check
                            !is_empty_string(data->p_rgx, this_match))
                    {
                        num_matches++;
                    }
                } while(this_match != NULL);

                //if matched == TRUE we should have at least one match
                assert(num_matches > 0);
                
                //warn the user about >1 match
                if(num_matches > 1)
                {
                    *err = (MapToOutputError) {
                        .err_type = MORE_THAN_ONE_DEVICE,
                        .err_msg = "More than one usable device was found in"
                            " wacom_parse_device_name"
                    };
                }

                //use the first match
                //512 is used as a sanity check
                const char* first_match = all_matches[0];
                ret = strndup(first_match, 512);

                //this frees the array AND frees the individual strings
                //which is why we needed to call strndup
                g_strfreev(all_matches);
            }
        }

        g_match_info_unref(match);

        return ret;
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
        *wacom_data = (WacomTabletData){0,0};
    }

    gchar* stdout_sink = NULL;

    wacom_data->p_rgx = init_parsing_regexes();
    if(wacom_data->p_rgx == NULL)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_REGEX_ERROR,
            .err_msg = "Error initializing regex structures for the wacom driver"
        };
        goto init_wacom_driver_error;
    }

    //run xsetwacom and get device names
    //TODO: double check how I'm supposed to get stdout back...
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
            XO_LOG_GERROR(cmd);
        }

        goto init_wacom_driver_error;
    }
    else
    {
        //process the driver output
        char* device_name = wacom_parse_device_name(wacom_data,
                stdout_sink, err);

        if(device_name == NULL)
        {
            goto init_wacom_driver_error;
        }
        else
        {
            wacom_data->device_name = device_name;
        }


        return wacom_data;
    }


init_wacom_driver_error:
    g_warn_if_reached();

    if(wacom_data != NULL)
    {
        free(wacom_data);
    }

    if(stdout_sink != NULL)
    {
        free(stdout_sink);
    }
    return NULL;
}

static void wacom_reset_map_to_output(void* v, MapToOutputError* err)
{
    WacomTabletData* wacom_data = (WacomTabletData*)v;
    if(wacom_data == NULL)
    {
        *err = (MapToOutputError){
            .err_type = RESET_MAP_TO_OUTPUT_FAILED,
            .err_msg = "passed driver data pointer was NULL"
        };
        return;
    }


    //build the command string
    const size_t cmd_buf_len = 4096;
    char cmd_buf[cmd_buf_len];

    //TODO: check snprintf return value to make sure the string wasn't too long
    //...would need a REALLY long device name
    snprintf(cmd_buf, cmd_buf_len, 
            "%s --set '%s' MapToOutput desktop",
            WACOM_DRIVER, wacom_data->device_name);

    //invoke the driver
    gint exit_code = -1;
    GError* gerr_ptr = NULL;
    gboolean res = g_spawn_command_line_sync(cmd_buf,
            NULL, NULL,
            &exit_code,
            &gerr_ptr);

    if(!res)
    {
        XO_LOG_GERROR(cmd_buf);

        *err = (MapToOutputError){
            .err_type = RESET_MAP_TO_OUTPUT_FAILED,
            .err_msg = "xsetwacom call failed"
        };
    }
}

void free_wacom_driver(void* v)
{
    WacomTabletData* wacom_data = (WacomTabletData*)v;

    if(wacom_data != NULL)
    {
        if(wacom_data->device_name != NULL)
        {
            free(wacom_data->device_name);
        }

        if(wacom_data->p_rgx != NULL)
        {
            free_parsing_regexes(wacom_data->p_rgx);
        }

        free(wacom_data);
    }

    //TODO: call xsetwacom w/MapToOutput arg "desktop"
}

/**
 * TODO: fill in fields
 */
const TabletDriver wacom_driver = {
    .init_driver = &init_wacom_driver,
    .free_driver = &free_wacom_driver
};
