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
void init_global_routes(const char* routes_fn) {
    char line[1024]; //XXX DANGEROUS, assumes shortish lines
    char stops[256][128]; // XXX less but STILL DANGEROUS

    int start_time = 0;
    int end_time = 0;
    char* curr;
    char* newline;
    int route_steps = 0;

    bool start_line = false;
    bool count_line = false;

    int total_routes = 0;
    int curr_route = 0;
    int stop_counter;
    char *line_holder, *end, *token;

    int i;

    // TODO: Add the path as a proper arg
    FILE* dat_file = fopen(routes_fn, "r"); 
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
        // Strip the newline
        newline = strchr(line, '\n');
        if (newline != NULL) {
            *newline = 0;
        }
        // 0 out the stop counter
        stop_counter = 0;


        // Now tokenize and get the path
        curr = strtok(line, " ");
        while (curr != NULL) {

            // Process the current line
            line_holder = end = strdup(curr);

            // That first one is the station
            token = strsep(&end, ",");
            strcpy(stops[stop_counter], token);

            // That second one is the time
            token = strsep(&end, ",");
            if (start_time == 0) {
                start_time = atoi(token);
            }
            end_time = atoi(token);
            // Clean up our strsep mess
            free(line_holder);

            // Bump the counter
            stop_counter++;
            // Get the next one 
            curr = strtok(NULL, " ");
        }
        // On the last interation, we dont move to next stop
        stop_counter--;

        // Ok, we've copied all the stops into a temp array, now go ahead and
        // put together a global list for init
        (route_list[curr_route]).start_time = start_time;
        (route_list[curr_route]).end_time = end_time;
        (route_list[curr_route]).length = stop_counter + 1;
        (route_list[curr_route]).stops = (char**)calloc(stop_counter +1, sizeof(char*));
        for(i=0; i < stop_counter+1; i++) {
            // Make some space for that name 
            (route_list[curr_route]).stops[i] = (char *)calloc(strlen(stops[i]) + 1, sizeof(char)); 
            // Copy it
            strcpy((route_list[curr_route]).stops[i], stops[i]);
        }
        (route_list[curr_route]).next_route = NULL;

        // Clear out start time for the next one
        start_time = 0;
        // Bump the index for the current route
        curr_route++;
    }

    g_total_routes = total_routes;
    g_total_transit_units = allocate_transit_units(); 

    return;
}

/* 
 * Make a new, blank, route set.
 */
route_set_t* create_set() {
    route_set_t* new_set;

    new_set = (route_set_t*)malloc(sizeof(route_set_t));
    if (new_set == NULL) {
        perror("Failed to allocate new route list!\n");
        exit(1);
    }
    
    new_set->first_route = NULL;
    new_set->last_route = NULL;
    new_set->curr_end = 0;
    new_set->next = NULL;

    return new_set;
}

/* 
 * Add a route to this set.
 */
int add_route(route_set_t* curr_set, abstract_route_t* new_route){
    // Can we add it?
    if (new_route->start_time < (curr_set->curr_end + MIN_ROUTE_GAP)) {
        return 1;
    }

    // Actually staple it to the end   
    if (curr_set->first_route == NULL) {
        curr_set->first_route = new_route;
        curr_set->last_route = new_route;
    } else {
        curr_set->last_route->next_route = new_route;
    }
    
    // Bump the endings
    curr_set->last_route = new_route;
    curr_set->curr_end = new_route->end_time;

    return 0;
}

/*
 * Spin through the routes and assign them to transit units. For now. just do it 
 * greedily from the front, for simplicity
 */
int allocate_transit_units() {
    int num_transit_units = 0;
    int i=0;
    route_set_t* curr_set = NULL;


    // The idea is that someday I can replace this with something that
    // implements a heuristic for assigning physical vehicles to runs.
    // For now, its silly, but easy to implement.

    // Start off with one
    route_set_list = create_set();

    // Spin through all the routes and add them to the first
    // set that will take them. Make a new one if none will.
    for(i=0; i < g_total_routes; i++) {
        curr_set = route_set_list; 
      
        while(add_route(curr_set, &route_list[i]) != 0) {
            // Are we at the end of the list?
            if (curr_set->next == NULL) {
                // Allocate a new one
                curr_set->next = create_set();
                // Since we needed a new one, that amounts to an additional
                // transit unit, so bump the count
                num_transit_units++;
            }
            curr_set = curr_set->next;
        }
    }

    return num_transit_units;
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

void  set_route_offset(int offset) {
    route_offset = offset;
}

int get_route_count() {
    return g_total_routes;
}

int get_transit_unit_count(){
    return g_total_transit_units;
}

abstract_route_t* get_route(int id) {
    //TODO: safety
    // have this select from the list of pre-assigned route-sets
    return &(route_list[id - route_offset]);
}

int get_g_start_time() {
    return g_start_time;
}

/************ Route Objects *********************/

route_t* init_route(char** steps, int len) {
    route_t* route_obj;
    int i;

    if (len == 0) {
        perror("Invalid route length!\n");
    }

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
    route_obj->start_dir = (route_obj->route)[1] - (route_obj->route)[0];
    route_obj->terminal = (route_obj->route)[len-1];

    route_obj->next_route = NULL;

    return route_obj;
}


/* Get the next stop along the route */
long int get_next(route_t* route_obj, int* current){

    int i;
    int* route;

    // If the next one is past the  edge, just go home
    if (((*current) + 1)  == route_obj->length) {
        return -1;
    }
    return route_obj->route[(*current)+1];
}
