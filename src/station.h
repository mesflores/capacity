// station.h
// type definitions for the station LP

#ifndef _station_h
#define _station_h

/**************** Station LP ***************/

// Station state
typedef enum {
    ST_EMPTY, // Station is empty, let trains in
    ST_OCCUPIED, // There is a train here, alighting passengers
    ST_BOARDING, // Loading passengers onto the TU
} station_sm; //SM - state machine, not full state

// Wraps up the track state
typedef struct {
    station_sm inbound;
    station_sm outbound;

    int track_id; // Just an identifier so we can tell them apart

    tw_lpid curr_tu; // who is on the track now

    short add_board; // there was additional boarding

    // Inbound queue
    unsigned short queued_tu_present;
    tw_lpid queued_tu[QUEUE_LEN];

    // Outbound queue
    // This really represents the space between the stations`
    int next_arrival;

    short from_queue;

} track_t;

typedef struct {
    char station_name[25]; // Identifier name for station TODO FIX SIZE

    //station_sm curr_state; // What state is it in right now?

    // The left track is where the station ID of the adjacent station is less
    // The righttrack is where the station ID of the adjacent station is more
    track_t left;
    track_t right;

    //unsigned short queued_tu_present; // Anything in the queue?
    //tw_lpid queued_tu; // The TU queued up. TODO: For now a single int, should get expanded

    passenger_t* pass_list; //Passenger linked list
} station_state;

//Global variables used by both main and driver
// - this defines the LP types
extern tw_lptype station_lps[];

//Function Declarations
// defined in station.c:
void station_init(station_state *s, tw_lp *lp);
void station_event(station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void station_event_reverse(station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void station_final(station_state *s, tw_lp *lp);

// Tracing
extern void station_ev_trace(void *msg, tw_lp *lp, char *buffer, int *collect_flag);
extern void station_stat_collect(state *s, tw_lp *lp, char *buffer);

#endif
