// route.c
//
// route.c
// Functions for managing routes

#include "ross.h"
#include "route.h"

#include <stdio.h>

route_t* init_route(int* steps, int* delays, int len) {
    route_t* route_obj;

    // Alloate the route struc itself
    route_obj = tw_calloc(TW_LOC, "init_route", sizeof(route_t), 1);
    // ok, now allocate and copy the steps and delays
    route_obj->route = tw_calloc(TW_LOC, "init_route", sizeof(int), len);
    route_obj->delay = tw_calloc(TW_LOC, "init_route", sizeof(int), len);

    // Copy the lists into there
    memcpy(route_obj->route, steps, sizeof(int) *len);
    memcpy(route_obj->delay, delays, sizeof(int) *len);

    route_obj->length = len;

    // Set the origin and terminal based on what we were given
    route_obj->origin = steps[0];
    route_obj->terminal = steps[len-1];

    return route_obj;
}


/* Get the next stop along the route */
int get_next(route_t* route_obj, int* current){

    int i;
    int* route;

    // If the enxt one is past the  edge, just go home
    if (((*current) + 1)  == route_obj->length) {
        return -1;
    }
    return route_obj->route[(*current)+1];
}
