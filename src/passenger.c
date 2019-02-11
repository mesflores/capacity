// passenger.c
//
// Code for managing the passenger

#include "ross.h"
#include "passenger.h"

/* Make a new passenger*/
passenger_t* create_passenger(int start, int dest, float time){
    passenger_t* curr_pass;

    // Gets get a chunk of mem
    curr_pass = tw_calloc(TW_LOC, "create_passenger", sizeof(passenger_t), 1);
    curr_pass->start = start;
    curr_pass->dest = dest;
    curr_pass->time = time;

    curr_pass->state = WAIT; // YOu start waiting

    curr_pass->next = NULL; // Not in the list for station yet

    return curr_pass;
}

int should_board(passenger_t* curr_pass) {
    // XXX: Passengers really need a way to ask if  this train is would
    // take them to the right place. In the original, we'd store a full route
    // and ask if this train was going to the next hop, but I  think that's stupid
    // for now, get on every train

    return 1;
}

