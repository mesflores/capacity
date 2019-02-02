// route.c
//
// route.c
// Functions for managing routes

#include "graph.h"
#include "ross.h"
#include "route.h"

#include <stdio.h>

route_t* init_route(char** steps, int len) {
    route_t* route_obj;
    int i;

    // Alloate the route struc itself
    route_obj = tw_calloc(TW_LOC, "init_route", sizeof(route_t), 1);
    // ok, now allocate and copy the steps
    route_obj->route = tw_calloc(TW_LOC, "init_route", sizeof(char*), len);
    for (i=0; i<len; i++) {
        // Copy the name into there
        (route_obj->route)[i] = sta_id_lookup(steps[i]);
    }


    route_obj->length = len;

    // Set the origin and terminal based on what we were given
    route_obj->origin = (route_obj->route)[0];
    route_obj->terminal = (route_obj->route)[len-1];

    return route_obj;
}


/* Get the next stop along the route */
int get_next(route_t* route_obj, int* current){

    int i;
    int* route;

    // If the next one is past the  edge, just go home
    if (((*current) + 1)  == route_obj->length) {
        return -1;
    }
    return route_obj->route[(*current)+1];
}
