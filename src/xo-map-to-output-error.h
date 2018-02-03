#pragma once

/****error types*****/
enum MapToOutputErrorType {
    NO_ERROR = 0,
    BAD_MALLOC,
    DRIVER_GET_TABLET_DIMENSIONS
};

typedef struct MapToOutputError {
    enum MapToOutputErrorType err_type;
    char* err_msg;
} MapToOutputError;


//TODO: add more error structs
static MapToOutputError no_error {
    .err_type = NO_ERROR,
    .err_msg = "Success"
};

static MapToOutputError bad_malloc {
    .err_type = BAD_MALLOC,
    .err_msg = "malloc returned NULL"
};
