
// model.h
// type definitions for all the LP types

#ifndef _model_h
#define _model_h

/********* Shared components across LPs *****/
typedef enum {
    TRAIN_ARRIVE,
    TRAIN_DEPART
} message_type;

// Message 
typedef struct {
    message_type type;
    tw_lpid origin;
    passenger curr_pass;
} message;

/*******************************************/
/**************** Station LP ***************/

// Station state
typedef struct {
    int p_arrive; // Passangers that arrived here
    int p_depart; // Passangers that left
    passenger curr_pass; //TODO Currently allows one passanger per station
} state;

//Global variables used by both main and driver
// - this defines the LP types
extern tw_lptype station_lps[];

//Function Declarations
// defined in station.c:
extern void station_init(state *s, tw_lp *lp);
extern void station_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_final(state *s, tw_lp *lp);
// defined in model_map.c:
extern tw_peid station_map(tw_lpid gid);

/*******************************************/
/************* Transit Unit LP *************/

// TU state
typedef struct {
    int station; // GID of current station
    int status; // Some kind of state machine?
    // This should be an array of some kind
} tu_state;

//Function Declarations
// defined in transit_unit.c:
extern void transit_unit_init(tu_state *s, tw_lp *lp);
extern void transit_unit_event(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_event_reverse(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_final(tu_state *s, tw_lp *lp);

extern tw_peid transit_unit_map(tw_lpid gid);

/*******************************************/
#endif
