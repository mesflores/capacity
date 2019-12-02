 // route.h
 // the route objects

#ifndef _route_h
#define _route_h

/********* Global route Management *******/

typedef struct route_t {
    char **stops; //Station names
    int* route; // The set of stations themselves
    int length;
    int start_time;
    int end_time;

    // Some more meta information
    int origin;
    int start_dir;
    int terminal;

    struct route_t* prev_route;
    struct route_t* next_route;
} route_t;

/*
 * A linked list of route sets
 */
typedef struct route_set_t {
    struct route_t* first_route;
    struct route_t* last_route;
    int curr_end;
    struct route_set_t* next;
} route_set_t;
// Functions for building route sets;
route_set_t* create_set();
int add_route(route_set_t* curr_set, route_t* new_route);


/*******/

void init_global_routes(const char* routes_fn);

int allocate_transit_units();

void print_global_routes();
void set_route_offset(int offset);
int get_route_count();
int get_transit_unit_count();
int get_g_start_time();
route_t* get_route(int id);

long int get_next(route_t* route_obj, int* current);



#endif

