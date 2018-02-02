#pragma once

#include "xo-map-to-output-error.h"

typedef struct TabletDriver {
    void (*get_tablet_dimensions)(unsigned int*, unsigned int, MapToOutputError*);
} TabletDriver;
