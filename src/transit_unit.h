// transit_unit.h
// Type definitions for Transit Unit (TU) LP

#ifndef _transit_unit_h
#define _transit_unit_h

/************* Transit Unit LP *************/

typedef enum {
    TU_IDLE, // Starting state only
    TU_APPROACH, // Approaching a station
    TU_ALIGHT, // At a station, empty pass
    TU_BOARD, // At a station, barding pass
} transit_unit_sm; //sm - state machine, diff from state

// TU state
typedef struct {
    transit_unit_sm curr_state; // Some kind of state machine?
    int start; // time to start
    tw_lpid prev_station; // GID of previous station
    tw_lpid station; // GID of current station
    int route_index; // Current index in route
    // This should be an array of some kind
    struct route_t* route;
    // TODO Almost certainly we want to sort these by where they are getting off...
    // for now, a big stupid linked list
    passenger_t* pass_list; //Passenger linked list
    int pass_count;

    int min_time; // the minimum arrival time at next station

} tu_state;

//Function Declarations
// defined in transit_unit.c:
void transit_unit_init(tu_state *s, tw_lp *lp);
void transit_unit_pre_run(tu_state *s, tw_lp *lp);
void transit_unit_event(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void transit_unit_event_reverse(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void transit_unit_final(tu_state *s, tw_lp *lp);



extern int* route_init(); // Init the route

int initial_approach(tu_state* s,  tw_lp *lp, int init);
int advance_route(tu_state *s, tw_lp *lp);

#endif
