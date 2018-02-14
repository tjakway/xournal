#pragma once

/****error types*****/
enum MapToOutputErrorType {
    NO_ERROR = 0,
    BAD_MALLOC,
    DRIVER_GET_TABLET_DIMENSIONS,
    CANVAS_INIT_ERROR,
    ARG_CHECK_FAILED,

    NO_MAPPING_ERROR,

    NEED_NEW_PAGE,

    NO_OUTPUT_BOX,
    SCROLL_PLAN_ERROR,

    GLIB_THREAD_ERROR,

    /** a gdk call failed or returned nonsensical results 
     * can't use GDK_ERROR because it's already defined by GDK... */
    MAPTOOUTPUT_GDK_ERROR,
    WIDGET_DIMENSION_ERROR,

    //tablet driver errors
    DRIVER_PROGRAM_NOT_FOUND, 
    NO_DEVICE_FOUND,
    MORE_THAN_ONE_DEVICE,
    RESET_MAP_TO_OUTPUT_FAILED,

    WACOM_REGEX_ERROR,
    WACOM_GET_AREA_ERROR,
    WACOM_GET_AREA_PARSING_ERROR,
    WACOM_BAD_DIMENSIONS,
    WACOM_CALL_MAP_TO_OUTPUT_ERROR
};

typedef struct MapToOutputError {
    enum MapToOutputErrorType err_type;
    /**
     * a statically allocated error message
     */
    const char* err_msg;
} MapToOutputError;


#define MAP_TO_OUTPUT_STRINGIFY__(x) #x
#define MAP_TO_OUTPUT_TO_STRING(x) MAP_TO_OUTPUT_STRINGIFY__(x)
#define LINE_STR MAP_TO_OUTPUT_TO_STRING(__LINE__)

#define MAP_TO_OUTPUT_ERROR_BAD_MALLOC (MapToOutputError) { \
        .err_type = BAD_MALLOC, \
        .err_msg = ("malloc returned NULL, reported on line " LINE_STR " of file " __FILE__)  \
    };

#define ERR_OK(x) (x != NULL && x->err_type == NO_ERROR)


#define XO_LOG_GERROR(desc) do {  \
            const char* msg = gerr_ptr != NULL ? gerr_ptr->message \
                    : "gerr_ptr == NULL, no glib error string available"; \
            g_log(NULL, G_LOG_LEVEL_WARNING, \
                    "error in %s: `%s`, " \
                    "glib reports error as: %s\n", \
                    __func__, desc, msg); \
            } while(0);

void map_to_output_warn_if_error(MapToOutputError*);

extern const MapToOutputError no_error;
