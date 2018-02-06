#pragma once

#include "xo-map-to-output-error.h"
#include "xo-tablet-driver.h"


#define WACOM_DRIVER "xsetwacom"

typedef struct WacomTabletData {
    const char* device_name;

} WacomTabletData;


const MapToOutputError driver_program_not_found = {
    .err_type = DRIVER_PROGRAM_NOT_FOUND,
    .err_msg = "Could not find tablet driver named " WACOM_DRIVER
};


void* init_wacom_driver(MapToOutputError*);

/**
 * parse the result of xsetwacom --list devices or
 * return the device name or NULL if none is found
 */
char* wacom_parse_device_name(const char*);
