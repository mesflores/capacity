// passenger.h
// type definitions for the passenger object

#ifndef _passenger_h
#define _passenger_h

typedef enum {
    UNINIT,
    WAIT,
    RIDE,
    TRANSFER
} passenger_state;

typedef struct passenger_t {
    int start; // Station ID of start
    int dest; // Station ID of destination

    float time; // Last recorded event

    passenger_state state; // Current state of passenger

    // next one in the list
    struct passenger_t* next;
} passenger_t;

passenger_t* create_passenger(int start, int dest, float time);

int should_board(passenger_t* curr_pass);

#endif
