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
    WIDGET_DIMENSION_ERROR
};

typedef struct MapToOutputError {
    enum MapToOutputErrorType err_type;
    char* err_msg;
} MapToOutputError;


//TODO: add more error structs
const MapToOutputError no_error = {
    .err_type = NO_ERROR,
    .err_msg = "Success"
};

const MapToOutputError bad_malloc = {
    .err_type = BAD_MALLOC,
    .err_msg = "malloc returned NULL"
};

