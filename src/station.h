// station.h
// type definitions for the station LP

#ifndef _station_h
#define _station_h

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
#endif