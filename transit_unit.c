// transit_unit.c
//
// Functions for the transit_unit LP

//Includes
#include <stdio.h>

#include "ross.h"
#include "passenger.h"
#include "model.h"

//Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void transit_unit_init (tu_state *s, tw_lp *lp) {

    // Init the train state
    // TODO: At some point schedule initialization will be a complicated question
    s->curr_state = TU_IDLE; // Starts nowhere

    s->station = -1; // XXX I dunno, we'll figure it out XXX

    return;
}

//Pre-run handler
void transit_unit_pre_run (tu_state *s, tw_lp *lp) {
    int self = lp->gid;

    // Send an approach message to the first station on the schedule
    // For now, that's (very dubiously) hard coded at 0
    tw_event *e = tw_event_new(0, 0, lp);
    message *msg = tw_event_data(e);
    msg->type = TRAIN_ARRIVE;
    // All these passengers got on here I guess
    msg->source = self;
    tw_output(lp, "Sending arrive message to 0!\n");
    tw_event_send(e);

    // Update your own state
    s->curr_state = TU_APPROACH;

    return;
}


//Forward event handler
void transit_unit_event (tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;
    int dest;
    int curr_global;
    
    int delay;
    int next_station;


    // initialize the bit field
    // TODO: Understand what these are for...
    *(int *) bf = (int) 0;

    // update the current state
    // however, save the old value in the 'reverse' message
    // SWAP(&(s->last_arr), &(in_msg->passenger_count));

    // handle the message
    switch (in_msg->type) {
        case ST_ACK : {
            // A station said we are good to go!
            // Go ahead and put yourself in alight mode 
            s->curr_state = TU_ALIGHT; 

            // TODO: Send messages to station to empty passengers

            // For now, no passengers go right to boarding
            tw_event *e = tw_event_new(in_msg->source, 0, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_BOARD;
            msg->source = self;
            tw_output(lp, "[%f] Sending alighting complete to %d!\n", tw_now(lp), in_msg->source);
            tw_event_send(e);

            // Train is now accepting boarding passengers
            s->curr_state = TU_BOARD;

            break;
        }
        case P_COMPLETE : {
            // station is all done boarding 
            // Ok all done go ahead and depart
            tw_event *e = tw_event_new(in_msg->source, 0, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_DEPART;
            msg->source = self;
            tw_output(lp, "[%f] Sending depart to %d!\n", tw_now(lp), in_msg->source);
            tw_event_send(e);

            // Ok tell the next station that we are on our way 
            // TODO figure out the next station properly
            next_station = in_msg->source + 1;
            // Time it takes to get to the next station
            delay = 10;
            // Send it an approach message
            tw_event *approach = tw_event_new(next_station, delay, lp);
            message *next_msg = tw_event_data(approach);
            next_msg->type = TRAIN_ARRIVE;
            next_msg->source = self;
            tw_output(lp, "[%f] Sending approach to %d!\n", tw_now(lp), next_station);
            tw_event_send(approach);
            break;
        }
        default :
            printf("Unhandeled forward message type %d\n", in_msg->type);
    }


    return;
}


//Reverse Event Handler
void transit_unit_event_reverse (tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    return;
}

//report any final statistics for this LP
void transit_unit_final (tu_state *s, tw_lp *lp){
    return;
}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid transit_unit_map(tw_lpid gid){
    return (tw_peid) gid / g_tw_nlp;
}

