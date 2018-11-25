// station.c
//
// Functions for the Station LP

//Includes
#include <stdio.h>

#include "ross.h"
#include "passenger.h"
#include "model.h"

//Helper Functions
void SWAP (int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

//Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void station_init (station_state *s, tw_lp *lp) {
    int self = lp->gid;

    // init state data
    s->p_arrive = 0;
    s->p_depart = 0;

    // IF you're the first station, make a passenger
    if ((g_tw_mynode == 0) && (self == 0)) {
        s->curr_pass.start = self;
        s->curr_pass.dest = (tw_nnodes() * g_tw_nlp) - 1;
        s->curr_pass.state = WAIT;
     
    // Otherwise create one with dumb settings for now
    } else {
        s->curr_pass.start = 0;
        s->curr_pass.dest = 0;
        s->curr_pass.state = UNINIT;
        
    }

    // Init message to myself
    // Will kick off everything else
    if ((g_tw_mynode == 0) && (self == 0)) {
        tw_event *e = tw_event_new(self, 1, lp);
        message *msg = tw_event_data(e);
        msg->type = TRAIN_ARRIVE;
        // All these passengers got on here I guess
        msg->origin= self;
        tw_event_send(e);
    }
}

//Forward event handler
void station_event (station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;
    int dest;
    int curr_global;

    // initialize the bit field
    // TODO: Understand what these are for...
    *(int *) bf = (int) 0;

    // update the current state
    // however, save the old value in the 'reverse' message
    // SWAP(&(s->last_arr), &(in_msg->passenger_count));

    // handle the message
    switch (in_msg->type) {
        case TRAIN_ARRIVE : {
            tw_output(lp, "[%f] Train arriving at %d!\n", tw_now(lp), self);
            // TODO: Currently the train only holds one person...
            s->p_arrive += 1;

            // Go ahead and move the passenger into the local state
            s->curr_pass.start = in_msg->curr_pass.start;
            s->curr_pass.dest = in_msg->curr_pass.dest;
            s->curr_pass.state = in_msg->curr_pass.state;

            // Where is this station? If its a terminal break here
            curr_global = tw_nnodes() * g_tw_nlp;
            // If we are the last one, generate nothing and return
            if (self == curr_global - 1) {
                break;
            }
        
            // Schedule a train departure in the near future
            tw_event *e = tw_event_new(self, 1, lp);
            message *msg = tw_event_data(e);

            msg->type = TRAIN_DEPART;
            // The rest is garbage right now, so let's  just ignore
            tw_event_send(e);

            break;
        }
        case TRAIN_DEPART : {
            tw_output(lp, "[%f] Train Departing %d\n", tw_now(lp), self);

            // Schedule an arrival at the next station
            // This should be safe here, since trains will never depart from a terminal
            dest = self + 1; // This is safe here because we returned the last guy
            tw_event *e = tw_event_new(dest, 2, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_ARRIVE;
            msg->origin = self;
            // Pass should board the train
            msg->curr_pass.start = s->curr_pass.start;
            msg->curr_pass.dest = s->curr_pass.dest;
            msg->curr_pass.state = s->curr_pass.state;
            // Clear out the state
            s->curr_pass.state = UNINIT;

            // Ship it off!
            tw_event_send(e);
            break;


        }
        default :
            printf("Unhandeled forward message type %d\n", in_msg->type);
    }

}


//Reverse Event Handler
void station_event_reverse (station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;


    /*
    // undo the state update using the value stored in the 'reverse' message
    SWAP(&(s->last_arr), &(in_msg->passenger_count));

    // handle the message
    switch (in_msg->type) {
        case TRAIN_ARRIVE:
            {
                s->p_arrive -= in_msg->passenger_count;
                break;
            }
        default :
            printf("Unhandeled reverse message type %d\n", in_msg->type);
    }

    // don't forget to undo all rng calls
    tw_rand_reverse_unif(lp->rng);
    */
    printf("Not Implemented!");
}

//report any final statistics for this LP
void station_final (station_state *s, tw_lp *lp){
    int self = lp->gid;
    printf("Station %d handled %d passengers\n", self, s->p_arrive);
}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid station_map(tw_lpid gid){
    return (tw_peid) gid / g_tw_nlp;
}

