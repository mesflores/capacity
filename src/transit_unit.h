// transit_unit.h
// Type definitions for Transit Unit (TU) LP

#ifndef _transit_unit_h
#define _transit_unit_h

// TU state
typedef struct {
    int station; // GID of current station
    int status; // Some kind of state machine?
    // This should be an array of some kind
} tu_state;

//Function Declarations

// defined in transit_unit.c:

extern int* route_init(); // Init the route

int initial_approach(tu_state* s,  tw_lp *lp, int init);
int advance_route(tu_state *s, tw_lp *lp);

// Ross functions
extern void transit_unit_init(tu_state *s, tw_lp *lp);
extern void transit_unit_event(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_event_reverse(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_final(tu_state *s, tw_lp *lp);

extern tw_peid station_map(tw_lpid gid);
#ndif
