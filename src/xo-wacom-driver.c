#include "xo-wacom-driver.h"
#include "xo-map-to-output.h"
#include "xo-tablet-driver.h"
#include "xo-map-to-output-error.h"

#include <glib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <limits.h>
#include <errno.h>
#include <inttypes.h> //for strtoimax, C99+ only

/**
 * Some notes about the wacom driver (xsetwacom):
 *  -MapToOutput is a write-only property that cannot be queried with xsetwacom --get
 *  -The format of Area is "x y width height"
 *
 */

#define MATCH_ONLY_WHITESPACE_RGX "/\\A\\s*\\z/"
#define MATCH_STYLUS_RGX "^Wacom[\\d\\w\\s]+Pen stylus(?=\\s+id: \\d+\\s+type: STYLUS\\s*$)"
#define MATCH_TABLET_DIMENSIONS_RGX "^(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*$"
#define MATCH_TABLET_DIMENSIONS_RGX_NUM_GROUPS 4


#define XO_LOG_GERROR(cmd) do {  \
            const char* msg = gerr_ptr != NULL ? gerr_ptr->message \
                    : "gerr_ptr == NULL, no glib error string available"; \
            g_log(NULL, G_LOG_LEVEL_WARNING, \
                    "error in %s during call to tablet driver: `%s`, " \
                    "glib reports error as: %s\n", \
                    __func__, cmd, msg); \
            } while(0);


static void free_parsing_regexes(struct WacomParsingRegexes*);

static void parse_tablet_dimensions(WacomTabletData* wacom_data,
        const char* str,
        int* out_x, int* out_y, 
        int* out_width, int* out_height,
        MapToOutputError* err);

static void run_and_parse_tablet_dimensions(
        WacomTabletData* wacom_data,
        int* out_x, int* out_y, 
        int* out_width, int* out_height,
        MapToOutputError* err);

WacomTabletData* init_wacom_tablet_data(MapToOutputError* err);
void wacom_get_device_name(WacomTabletData* wacom_data,
        MapToOutputError* err);

/**
 * parse a string to an int and store any errors in the
 * MapToOutputError pointer
 */
static int parse_str_to_int(char* to_parse, MapToOutputError* err)
{
    char* endptr = NULL;
    intmax_t res = strtoimax(to_parse, &endptr, 10);
    if(endptr == to_parse)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (nptr == endptr after call to strtoimax)" 
        };
        return -1;
    }
    //too large or too small to parse into the return type
    else if(res == INTMAX_MAX && errno == ERANGE)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (integer overflow from strtoimax)" 
        };
        return -1;
    }
    else if(res == INTMAX_MIN && errno == ERANGE)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (integer underflow from strtoimax)" 
        };
        return -1;
    }
    //parsing was successful but the result was too large or too small to store in an int
    else if(res > INT_MAX)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (returned value >INT_MAX)" 
        };
        return -1;
    }
    else if(res < INT_MIN)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (returned value <INT_MIN; also: should not be negative)" 
        };
        return -1;
    }
    //parsing successful and no overflow/underflow errors
    else
    {
        return (int)res;
    }
}



static struct WacomParsingRegexes* init_parsing_regexes()
{
    struct WacomParsingRegexes* p_rgx = malloc(sizeof(struct WacomParsingRegexes));
    if(p_rgx == NULL)
    {
        return NULL;
    }
    else
    {
        *p_rgx = (struct WacomParsingRegexes){ 0,0,0 };
    }

    const GRegexCompileFlags compile_flags = 
        //match across newlines and make . match newlines
        G_REGEX_DOTALL | 
        G_REGEX_MULTILINE |
        G_REGEX_CASELESS;


    GError* err = NULL;
    p_rgx->match_only_whitespace = 
        g_regex_new(
            MATCH_ONLY_WHITESPACE_RGX,
            compile_flags, 0,
            &err);
    if(p_rgx->match_only_whitespace == NULL
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

    p_rgx->match_tablet_dimensions = 
        g_regex_new(
                MATCH_TABLET_DIMENSIONS_RGX,
                //don't need multiline for this one
                G_REGEX_DOTALL | G_REGEX_CASELESS,
                0,
                &err);

    if(p_rgx->match_tablet_dimensions == NULL
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
        if(p_rgx->match_only_whitespace != NULL)
        {
            g_regex_unref(p_rgx->match_only_whitespace);
        }

        if(p_rgx->match_stylus != NULL)
        {
            g_regex_unref(p_rgx->match_stylus);
        }

        if(p_rgx->match_tablet_dimensions != NULL)
        {
            g_regex_unref(p_rgx->match_tablet_dimensions);
        }

        free(p_rgx);
    }
}

static gboolean is_empty_string(struct WacomParsingRegexes* p_rgx, const char* str)
{
    return g_regex_match(p_rgx->match_only_whitespace,
            str,0, NULL);
}

static int get_num_matches(WacomTabletData* wacom_data, char** all_matches)
{
    int num_matches = 0;
    char* this_match = NULL;
    do {
        this_match = all_matches[num_matches];

        if(this_match != NULL && 
            //sanity check
            !is_empty_string(wacom_data->p_rgx, this_match))
        {
            num_matches++;
        }
    } while(this_match != NULL);

    return num_matches;
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
                num_matches = get_num_matches(data, all_matches);

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

void process_tablet_dimensions(
        WacomTabletData* wacom_data,
        MapToOutputError* err)
{
    run_and_parse_tablet_dimensions(wacom_data,
            &wacom_data->dimension_x,
            &wacom_data->dimension_y,
            &wacom_data->dimension_width,
            &wacom_data->dimension_height,
            err);
}

void* init_wacom_driver(MapToOutputError* err)
{
    WacomTabletData* wacom_data = init_wacom_tablet_data(err);

    wacom_get_device_name(wacom_data, err);
    if(err->err_type != NO_ERROR)
    {
        free(wacom_data);
        return NULL;
    }

    process_tablet_dimensions(wacom_data, err);
    if(err->err_type != NO_ERROR)
    {
        free(wacom_data);
        return NULL;
    }

    return (void*)wacom_data;
}


void wacom_get_device_name(WacomTabletData* wacom_data,
        MapToOutputError* err)
{
    gchar* stdout_sink;

    //run xsetwacom and get device names
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
    }
    else
    {
        assert(stdout_sink != NULL);

        //process the driver output
        char* device_name = wacom_parse_device_name(wacom_data,
                stdout_sink, err);

        if(device_name == NULL)
        {
            *err = (MapToOutputError){
                .err_type = NO_DEVICE_FOUND,
                .err_msg = "No device found in output of "
                    WACOM_DRIVER " --list devices"
            };
        }
        else
        {
            wacom_data->device_name = device_name;
        }
    }

    if(stdout_sink != NULL)
    {
        free(stdout_sink);
    }
}

WacomTabletData* init_wacom_tablet_data(MapToOutputError* err)
{
    g_warn_if_fail(err != NULL);

    WacomTabletData* wacom_data = malloc(sizeof(WacomTabletData));
    if(wacom_data == NULL)
    {
        if(err != NULL)
        {
            *err = MAP_TO_OUTPUT_ERROR_BAD_MALLOC;
        }
        return NULL;
    }
    else
    {
        *wacom_data = (WacomTabletData){0,0,0,0,0,0};
    }

    wacom_data->p_rgx = init_parsing_regexes();
    if(wacom_data->p_rgx == NULL)
    {
        *err = (MapToOutputError){
            .err_type = WACOM_REGEX_ERROR,
            .err_msg = "Error initializing regex structures for the wacom driver"
        };
        goto init_wacom_driver_error;
    }


    return wacom_data;

init_wacom_driver_error:
    g_warn_if_reached();

    if(wacom_data != NULL)
    {
        free(wacom_data);
    }

    return NULL;
}


static void run_and_parse_tablet_dimensions(
        WacomTabletData* wacom_data,
        int* out_x, int* out_y, 
        int* out_width, int* out_height,
        MapToOutputError* err)
{
    //warn if bad parameters are passed
    g_warn_if_fail(wacom_data != NULL);
    g_warn_if_fail(out_x != NULL);
    g_warn_if_fail(out_y != NULL);
    g_warn_if_fail(out_width != NULL);
    g_warn_if_fail(out_height != NULL);
    g_warn_if_fail(err != NULL);


    //give bad return values if the tablet driver pointer is bad
    if(wacom_data == NULL)
    {
        if(out_x != NULL)
        {
            *out_x = -1;
        }
        if(out_y != NULL)
        {
            *out_y = -1;
        }
        if(out_width != NULL)
        {
            *out_width = -1;
        }
        if(out_height != NULL)
        {
            *out_height = -1;
        }
    }


    //call the tablet driver
    gint exit_code = -1;
    GError* gerr_ptr = NULL;
    gchar* stdout_sink = NULL;

    const int cmd_buf_len = 2048;
    char cmd_buf[cmd_buf_len];

    snprintf(cmd_buf, cmd_buf_len, "%s --get '%s' Area",
            WACOM_DRIVER, wacom_data->device_name);

    gboolean res = g_spawn_command_line_sync(cmd_buf,
            &stdout_sink,
            NULL,
            &exit_code,
            &gerr_ptr);

    if(!res)
    {
        XO_LOG_GERROR(cmd_buf);
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_ERROR,
            .err_msg = "call to xsetwacom --get <device_name>"
                " Area failed with nonzero exit code"
        };
    }
    else if(stdout_sink == NULL || is_empty_string(wacom_data->p_rgx, stdout_sink))
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_ERROR,
            .err_msg = "call to xsetwacom --get <device_name>"
                " Area returned with exit code 0 and no stdout"
        };
    }
    else
    {
        //parse the output
        //any errors will be written directly into err
        parse_tablet_dimensions(wacom_data,
                stdout_sink,
                out_x, out_y, out_width, out_height,
                err);
    }

    if(stdout_sink != NULL)
    {
        free(stdout_sink);
    }
}

static void parse_tablet_dimensions(WacomTabletData* wacom_data,
        const char* str,
        int* out_x, int* out_y, 
        int* out_width, int* out_height,
        MapToOutputError* err)
{
    GMatchInfo* match = NULL;
    gboolean matched = g_regex_match(
            wacom_data->p_rgx->match_tablet_dimensions,
            str, 0, &match);

    if(matched)
    {
        gchar** all_matches = NULL;
        all_matches = g_match_info_fetch_all(match);

        if(all_matches != NULL)
        {
            int num_matches = 0;
            num_matches = get_num_matches(wacom_data, all_matches);

            //note: glib calls capturing groups subpatterns sometimes 
            //(more specifically, subpatterns are a superset of capturing groups)
            if(num_matches == (MATCH_TABLET_DIMENSIONS_RGX_NUM_GROUPS + 1))
            {
                
                //parse each capturing group
                int parse_results[MATCH_TABLET_DIMENSIONS_RGX_NUM_GROUPS];
                for(int i = 1; i < (MATCH_TABLET_DIMENSIONS_RGX_NUM_GROUPS+1); i++)
                {
                    parse_results[i-1] = -1;

                    //don't parse if we've already encountered an error
                    if(err->err_type == NO_ERROR)
                    {
                        parse_results[i-1] = parse_str_to_int(all_matches[i], err);
                    }
                }

                //copy the parse results to the output pointers to return them
                *out_x = parse_results[0];
                *out_y = parse_results[1];
                *out_width = parse_results[2];
                *out_height = parse_results[3];

                //don't forget to free the matches vector
                g_strfreev(all_matches);

                return;
            }
            else
            {
                *err = (MapToOutputError){
                    .err_type = WACOM_GET_AREA_PARSING_ERROR,
                    .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                        " (all_matches != MATCH_TABLET_DIMENSIONS_RGX_NUM_GROUPS"
                        " but g_regex_match_all returned true)"
                };
            }

        }
        else
        {
            *err = (MapToOutputError){
                .err_type = WACOM_GET_AREA_PARSING_ERROR,
                .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                    " (all_matches == NULL but g_regex_match_all returned true)"
            };
        }

        g_strfreev(all_matches);
    }
    else
    {
        *err = (MapToOutputError){
            .err_type = WACOM_GET_AREA_PARSING_ERROR,
            .err_msg = "error parsing output of xsetwacom --get <device_name> Area"
                " (no regex match)"
        };
    }

    //set in case of error
    if(out_x != NULL)
    {
        *out_x = -1;
    }
    if(out_y != NULL)
    {
        *out_y = -1;
    }
    if(out_width != NULL)
    {
        *out_height = -1;
    }
    if(out_height != NULL)
    {
        *out_height = -1;
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
}

static void wacom_get_tablet_dimensions(void* v, 
        int* out_w, int* out_h, 
        MapToOutputError* err)
{
    //we already saved the tablet dimensions in our init function,
    //just write them out to the passed pointers
    
    WacomTabletData* wacom_data = (WacomTabletData*)v;
    g_warn_if_fail(wacom_data != NULL);
    g_warn_if_fail(out_w != NULL);
    g_warn_if_fail(out_h != NULL);
    g_warn_if_fail(err != NULL);

    if(wacom_data->dimension_width <= 0 || 
            wacom_data->dimension_height <= 0)
    {
        if(out_w != NULL)
        {
            *out_w = wacom_data->dimension_width;
        }
        if(out_h != NULL)
        {
            *out_h = wacom_data->dimension_height;
        }

        *err = (MapToOutputError){
            .err_type = WACOM_BAD_DIMENSIONS,
            .err_msg = "Error in wacom_get_tablet_dimensions: "
                "dimension_width or dimension_height was <= 0,"
                " probably due to an error in init_wacom_driver"
        };
    }
    else
    {
        //do the actual assignment
        *out_w = wacom_data->dimension_width;
        *out_h = wacom_data->dimension_height;
    }
}


//does the actual call to set MapToOutput, abstracted over whether we're
//resetting it or not
static void call_map_to_output(
        WacomTabletData* wacom_data,
        unsigned int x,
        unsigned int y,
        unsigned int width,
        unsigned int height,
        MapToOutputError* err,
        gboolean reset)
{
    const int cmd_buf_len = 2048;
    char cmd_buf[cmd_buf_len];

    //reset the output mapping using the special string "desktop"
    if(reset)
    {
        snprintf(cmd_buf, cmd_buf_len, 
                "%s --set '%s' MapToOutput desktop",
                WACOM_DRIVER, wacom_data->device_name);
    }
    //create a new mapping, overwriting the old one
    else
    {
        //WARNING: it's easy to mix up X11 geometry strings...
        //the format is WIDTHxHEIGHT+X+Y
        //DO NOT PUT X AND Y FIRST!
        snprintf(cmd_buf, cmd_buf_len, 
                "%s --set '%s' MapToOutput '%ux%u+%u+%u'",
                WACOM_DRIVER, wacom_data->device_name,
                width, height, x, y);
    }

    gint exit_code = -1;
    GError* gerr_ptr = NULL;

    gboolean res = g_spawn_command_line_sync(cmd_buf,
            NULL,
            NULL,
            &exit_code,
            &gerr_ptr);

    if(res == FALSE)
    {
        XO_LOG_GERROR(cmd_buf);
        *err = (MapToOutputError){
            .err_type = WACOM_CALL_MAP_TO_OUTPUT_ERROR,
            .err_msg = "Call to xsetwacom to set MapToOutput parameter failed"
                " with nonzero exit code"
        };
    }
}

static void wacom_map_to_output(void* v,  
        MapToOutput* map_to_output,
        unsigned int x, unsigned int y,
        unsigned int width, unsigned int height,
        MapToOutputError* err)
{
    g_warn_if_fail(v != NULL);
    g_warn_if_fail(map_to_output != NULL);
    g_warn_if_fail(err != NULL);

    //only call the driver if we haven't already encountered an error
    //otherwise we might not realize there was an error in call_map_to_output
    //and make mapping_mode harder to track
    if(err->err_type == NO_ERROR)
    {
        WacomTabletData* wacom_data = (WacomTabletData*)v;

        call_map_to_output(wacom_data, x, y, width, height, err, FALSE);

        if(err->err_type == NO_ERROR)
        {
            map_to_output->mapping_mode = MAPPING_ACTIVE;
        }
    }
}


static void wacom_reset_map_to_output(void* v, 
        MapToOutput* map_to_output, 
        MapToOutputError* err)
{
    g_warn_if_fail(v != NULL);
    g_warn_if_fail(map_to_output != NULL);
    g_warn_if_fail(err != NULL);

    WacomTabletData* wacom_data = (WacomTabletData*)v;

    if(err->err_type == NO_ERROR)
    {
        //parameters are ignored when reset == TRUE
        call_map_to_output(wacom_data, 0, 0, 0, 0, err, TRUE);

        if(err->err_type == NO_ERROR)
        {
            map_to_output->mapping_mode = NO_MAPPING;
        }
    }
}


const TabletDriver wacom_driver = {
    .init_driver = &init_wacom_driver,
    .free_driver = &free_wacom_driver,
    .get_tablet_dimensions = &wacom_get_tablet_dimensions,
    .map_to_output = &wacom_map_to_output,
    .reset_map_to_output = &wacom_reset_map_to_output
};
