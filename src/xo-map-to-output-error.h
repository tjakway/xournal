#pragma once

/****error types*****/
enum MapToOutputErrorType {
    NO_ERROR = 0,
    BAD_MALLOC,
    DRIVER_GET_TABLET_DIMENSIONS,
    CANVAS_INIT_ERROR,
    ARG_CHECK_FAILED,

    NEED_NEW_PAGE,

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
    const char* err_msg;
} MapToOutputError;


#define MAP_TO_OUTPUT_ERROR_BAD_MALLOC (MapToOutputError) { \
        .err_type = BAD_MALLOC, \
        .err_msg = "malloc returned NULL, reported on line __LINE__  of file  __FILE__ "\
    };

void map_to_output_warn_if_error(MapToOutputError*);

extern const MapToOutputError no_error;
