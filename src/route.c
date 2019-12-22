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
        perror("Failed to open routes file!");
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
            route_list = (route_t* )calloc(total_routes, sizeof(route_t));
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
        (route_list[curr_route]).route = (int*)calloc(stop_counter +1, sizeof(int));
        for(i=0; i < stop_counter+1; i++) {
            // Make some space for that name 
            (route_list[curr_route]).stops[i] = (char *)calloc(strlen(stops[i]) + 1, sizeof(char)); 
            // Copy it
            strcpy((route_list[curr_route]).stops[i], stops[i]);
            // Make an array of the ID mapped versions
            (route_list[curr_route]).route[i] = sta_id_lookup(stops[i]);
        }

        // Set the origin and terminal based on what we were given
        (route_list[curr_route]).origin = (route_list[curr_route]).route[0];
        (route_list[curr_route]).start_dir = (route_list[curr_route]).route[1] - (route_list[curr_route]).route[0];
        (route_list[curr_route]).terminal = (route_list[curr_route]).route[stop_counter];
        (route_list[curr_route]).prev_route = NULL;
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
int add_route(route_set_t* curr_set, route_t* new_route){
    // Can we add it?
    // This criteria checks against:
    //  a) There are any routes in here
    //  b) The start time is within the MIN ROUTE GAP
    //if ((curr_set->curr_end !=0) && (new_route->start_time < (curr_set->curr_end + MIN_ROUTE_GAP))) {
    //    return 1;
    //}
    // This criteria requres:
    // 	a) That new route picks up at the same station
    // 	b) starts strictly after its arrival
    if ((curr_set->curr_end !=0) &&
        ((curr_set->last_route->terminal != new_route->origin) ||
        (new_route->start_time <= curr_set->curr_end))) {
        return 1; 
    }



    // Actually staple it to the end   
    if (curr_set->first_route == NULL) {
        curr_set->first_route = new_route;
        curr_set->last_route = new_route;
    } else {
        new_route->prev_route = curr_set->last_route;
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
    int num_transit_units = 1;
    int i=0;
    route_set_t* curr_set = NULL;
    route_set_t* prev_set = NULL;


    // The idea is that someday I can replace this with something that
    // implements a heuristic for assigning physical vehicles to runs.
    // For now, its silly, but easy to implement.

    // Start off with one
    route_set_list_l = create_set();

    // Spin through all the routes and add them to the first
    // set that will take them. Make a new one if none will.
    for(i=0; i < g_total_routes; i++) {
        curr_set = route_set_list_l; 
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

    // For ease of assigning the route sets to transit units, lets map them into an array
    route_set_list = (route_set_t*)calloc(num_transit_units, sizeof(route_set_t));
    curr_set = route_set_list_l;
    i=0;
    while(curr_set != NULL) {
        memcpy(&(route_set_list[i]), curr_set, sizeof(route_set_t));
        i++;
        prev_set = curr_set;
        curr_set = curr_set->next;
        free(prev_set);
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

route_t* get_route(int id) {
    if ((id - route_offset) > g_total_transit_units) {
        perror("Attempted to a fetch a bad route id!\n");
        exit(1);
    }
    // have this select from the list of pre-assigned route-sets
    return &(route_list[id - route_offset]);
}

int get_g_start_time() {
    return g_start_time;
}

/************ Route Objects *********************/

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
