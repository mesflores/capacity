// model_mappings.h

#include "ross.h"
#include "passenger.h"
#include "model.h"

// Multiple LP Types mapping function
//    Given an LP's GID
//    Return the index in the LP type array (defined in model_main.c)
tw_lpid model_typemap (tw_lpid gid) {
    // the girst g_num_stations are stations and the remaining are transit_units
    if (gid < g_num_stations) {
        return STATION;
    }
    else {
        return TRANSIT_UNIT;
    }
}
