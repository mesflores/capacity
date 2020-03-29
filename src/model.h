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
    short delayed;
    passenger_t curr_pass;
} message;

/*******************************************/
// defined in model_map.c:
extern tw_peid station_map(tw_lpid gid);

/*******************************************/

extern tw_peid transit_unit_map(tw_lpid gid);

/*******************************************/
#endif
