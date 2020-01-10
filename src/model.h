// model.h
// type definitions for all the LP types

#ifndef _model_h
#define _model_h

#define CONTROL_EPOCH .006

#define QUEUE_LEN 100

extern void SWAP (int *a, int *b);

/****** Custom mapping prototypes *******/
void model_custom_mapping_linear(void);
void model_custom_mapping_rr(void);
tw_lp * model_mapping_to_lp(tw_lpid lpid);
tw_lp * model_mapping_to_lp_rr(tw_lpid lpid);
extern tw_peid lp_map_linear(tw_lpid gid);
extern tw_peid lp_map_rr(tw_lpid gid);

/********* Shared components across LPs *****/
extern FILE * node_out_file;

extern int g_num_stations;
extern int g_num_transit_units;
extern int g_time_offset;

extern char g_adj_mat_fn[1024];
extern char g_routes_fn[1024];

enum type_vals {
    STATION,
    TRANSIT_UNIT
};

tw_lpid model_typemap (tw_lpid gid);

typedef enum {
    TRAIN_ARRIVE, // Train is approaching the station
    P_ARRIVE, // Passenger arriving at the station
    ST_ACK, // Station said ok
    P_ALIGHT, // Passenger getting off
    TRAIN_BOARD, // Train now allowing boarding
    P_BOARD, // Passenger boarding
    TRAIN_CONT_BOARD, // Continue boarding
    P_COMPLETE, // PAssengers are done boarding
    TRAIN_DEPART // Leave the station
} message_type;

// Message 
typedef struct {
    message_type type;
    tw_lpid source;
    int prev_station;
    short more; //For P_ messages, are there more coming?
    int next_arrival; // ST_ACK - the time the previous departing train will reach the next station
                      // TRAIN_DEPART - the time the train will reach the next station
    short from_queue; 
    passenger_t curr_pass;
} message;

/*******************************************/
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
    // The righttrack is where the station ID of the adjacent station is less
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
extern void station_init(station_state *s, tw_lp *lp);
extern void station_event(station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_event_reverse(station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void station_final(station_state *s, tw_lp *lp);
// defined in model_map.c:
extern tw_peid station_map(tw_lpid gid);

/*******************************************/
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
extern void transit_unit_init(tu_state *s, tw_lp *lp);
extern void transit_unit_pre_run(tu_state *s, tw_lp *lp);
extern void transit_unit_event(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_event_reverse(tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
extern void transit_unit_final(tu_state *s, tw_lp *lp);

extern tw_peid transit_unit_map(tw_lpid gid);

/*******************************************/
#endif
