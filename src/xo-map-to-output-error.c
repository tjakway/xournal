#include "xo-map-to-output-error.h"

const MapToOutputError no_error = {
    .err_type = NO_ERROR,
    .err_msg = "Success"
};

const MapToOutputError bad_malloc = {
    .err_type = BAD_MALLOC,
    .err_msg = "malloc returned NULL"
};
