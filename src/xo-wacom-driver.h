#pragma once

#include "xo-map-to-output-error.h"
#include "xo-tablet-driver.h"


#define WACOM_DRIVER "xsetwacom"

typedef struct WacomTabletData {
    char* device_name;

    struct WacomParsingRegexes {
        GRegex *match_only_whitespace,
               *match_stylus,
               *match_tablet_dimensions;
    } *p_rgx;
} WacomTabletData;


const MapToOutputError driver_program_not_found = {
    .err_type = DRIVER_PROGRAM_NOT_FOUND,
    .err_msg = "Could not find tablet driver named " WACOM_DRIVER
};


void* init_wacom_driver(MapToOutputError*);

/**
 * parse the result of xsetwacom --list devices
 * return the device name or NULL if none is found
 *
 * if >1 device is found, MapToOutputErr will be set to MORE_THAN_ONE_DEVICE and
 * the first one will be returned
 */
char* wacom_parse_device_name(void*, const char*, MapToOutputError*);
