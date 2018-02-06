#pragma once

/****error types*****/
enum MapToOutputErrorType {
    NO_ERROR = 0,
    BAD_MALLOC,
    DRIVER_GET_TABLET_DIMENSIONS,
    CANVAS_INIT_ERROR,
    ARG_CHECK_FAILED,

    /** a gdk call failed or returned nonsensical results 
     * can't use GDK_ERROR because it's already defined by GDK... */
    MAPTOOUTPUT_GDK_ERROR,
    WIDGET_DIMENSION_ERROR,

    //tablet driver errors
    DRIVER_PROGRAM_NOT_FOUND, 
    NO_DEVICE_FOUND,
    MORE_THAN_ONE_DEVICE,
    RESET_MAP_TO_OUTPUT_FAILED,

    WACOM_REGEX_ERROR
};

typedef struct MapToOutputError {
    enum MapToOutputErrorType err_type;
    char* err_msg;
} MapToOutputError;

extern const MapToOutputError no_error;
extern const MapToOutputError bad_malloc;
