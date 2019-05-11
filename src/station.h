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
track_t track_map(int curr_station, int prev_station);
track_t track_map_rev(int curr_station, int prev_station, message *in_msg);

// Track queue management
int add_train(tw_lpid new_train, track_t* track); // Add somebody new
int pop_head(track_t* track); // Take the train off the front
int add_train_head(tw_lpid new_train, track_t* track); //Add train to head, for reverse
int pop_tail(track_t* track); // Take train off the back, for reverse



extern void station_init(state *s, tw_lp *lp, satation_state *s);
extern void station_event(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_event_reverse(state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_final(state *s, tw_lp *lp);
// defined in model_map.c:
extern tw_peid station_map(tw_lpid gid);
#endif
