// route.c
//
// route.c
// Functions for managing routes

#include "graph.h"
#include "ross.h"
#include "route.h"
#include "route_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**************** Global Route Lookup Stuff ****/

// Read the config and init the global routes, return the number of runs
void init_global_routes() {
    char line[512]; //XXX DANGEROUS
    char stops[256][128]; // XXX less but STILL DANGEROUS
    int g_start_time;

    int start_time = 0;
    char* curr;
    char* newline;
    int route_steps = 0;

    bool start_line = false;
    bool count_line = false;

    int total_routes = 0;
    int curr_route = 0;
    int stop_counter;

    int i;

    // TODO: Add the path as a proper arg
    FILE* dat_file = fopen("/home/marcel/capacity/dat/routes.dat.max", "r"); 
    if (dat_file == NULL) {
        perror("Failed to open file!");
        exit(-1);
    }

    // Let's read the file!
    while (fgets(line, sizeof(line), dat_file)) {
        // If its the first line, it's the global start time, grab that
        if (start_line == false) {
            start_line = true;
            g_start_time = atoi(line);
            continue;
        }
        // Get the number of routes we'll have
        if (count_line == false) {
            count_line = true;
            total_routes = atoi(line);
            // Go ahead and allocate space for those bad boys
            route_list = (abstract_route_t* )calloc(total_routes, sizeof(abstract_route_t));
            continue;
        } 
        // Ok now, we are parsing the regular lines
        // Read the time first
        if (start_time == 0) {
            start_time = atoi(line);
            continue;
        }
        printf("%s", line);
        // Otherwise it's time to read the route itself!
        // Strip the newline
        newline = strchr(line, '\n');
        if (newline != NULL) {
            *newline = 0;
        }
        // 0 out the stop counter
        stop_counter = 0;

        // Now tokenize and get the path
        curr = strtok(line, " ");
        // Stash it
        strcpy(stops[stop_counter], curr);

        while (curr != NULL) {
            curr = strtok(NULL, " ");
            if (curr != NULL) {
                stop_counter++;
                strcpy(stops[stop_counter], curr);
            }
        }

        // Ok, we've copied all the stops into a temp array, now go ahead and
        // put together a global list for init
        (route_list[curr_route]).start_time = start_time;
        (route_list[curr_route]).length = stop_counter + 1;
        (route_list[curr_route]).stops = (char**)calloc(stop_counter +1, sizeof(char*));
        for(i=0; i < stop_counter+1; i++) {
            // Make some space for that name 
            (route_list[curr_route]).stops[i] = (char *)calloc(strlen(stops[i]), sizeof(char)); 
            // Copy it
            strcpy((route_list[curr_route]).stops[i], stops[i]);
        }

        // Clear out start time for the next one
        start_time = 0;
        // Bump the index for the current route
        curr_route++;
    }

    g_total_routes = total_routes;

    return;
}

/* 
 * Dump the global route table, so I can see if it what I think
 */
void print_global_routes() {
    int i=0;
    int j=0;

    for(i = 0; i < g_total_routes; i++) {
        printf("========\n");
        printf("Start: %d Len: %d\n", (route_list[i]).start_time, (route_list[i]).length);
        printf("Route: ");
        for(j = 0; j < (route_list[i]).length; j++) {
            printf("%s ", (route_list[i]).stops[j]);
        }
        printf("\n");
    }

    return;
}


/************ Route Objects *********************/

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
