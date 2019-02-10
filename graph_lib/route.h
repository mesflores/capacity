 // route.h
 // the route objects

#ifndef _route_h
#define _route_h

/********* Global route Management *******/

typedef struct abstract_route_t {
    char **stops;
    int length;
    int start_time;
} abstract_route_t;

void init_global_routes();
void print_global_routes();
void set_route_offset(int offset);
int get_route_count();
abstract_route_t* get_route(int id);


/************ TU Route Data *************/
typedef struct route_t {
    int* route; // The set of stations themselves
    int* delay;
    int length; // The length of the route

    // Some more meta information
    int origin;
    int terminal;
} route_t;

route_t* init_route(char** steps, int len);
int get_next(route_t* route_obj, int* current);



#endif

