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

typedef struct {
    int start; // Station ID of start
    int dest; // Station ID of destination
    passenger_state state; // Current state of passenger
} passenger;

#endif
