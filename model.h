
// model.h
// type definitions for all the LP types

#ifndef _model_h
#define _model_h

/********* Shared components across LPs *****/

extern int g_num_stations;
extern int g_num_transit_units;

enum type_vals {
    STATION,
    TRANSIT_UNIT
};

tw_lpid model_typemap (tw_lpid gid);

typedef enum {
    TRAIN_ARRIVE, // Train is approaching the statoon
    ST_ACK, // Station said ok
    ST_NACK, // Station said Im busy
    P_ALIGHT, // Passenger getting off
    TRAIN_BOARD, // Train now allowing boarding
    P_BOARD, // Passenger boarding
    TRAIN_CONT_BOARD, // Continue boarding
    TRAIN_DEPART // Leave the station
} message_type;

// Message 
typedef struct {
    message_type type;
    tw_lpid origin;
    short more; //For P_ messages, are there more coming?
    passenger curr_pass;
} message;

/*******************************************/
/**************** Station LP ***************/

// Station state

typedef enum {
    ST_EMPTY, // Station is empty, let trains in
    ST_OCCUPIED, // There is a train here, alighting passengers
    ST_BOARDING, // Loading passengers onto the TU
} station_sm; //SM - state machine, not full state

typedef struct {
    station_sm curr_state; // What state is it in right now?
    int p_arrive; // Passangers that arrived here
    int p_depart; // Passangers that left
    passenger curr_pass; //TODO Currently allows one passanger per station
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
    int station; // GID of current station
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
