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
    s->curr_state = ST_EMPTY; // Stations start empty
    s->queued_tu_present = 0;
    s->queued_tu = 0;



/*
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
*/
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
          
            // First, check to see what our state is
            if ((s->curr_state == ST_OCCUPIED) || (s->curr_state == ST_BOARDING)) {
                // If we are currently occupied, put the TU in the queue for 
                // notification when the state transitions to empty
                // Was anybody queued? if so hard error
                if (s->queued_tu_present > 0) {
                    printf("NOT IMPLEMENTED: proper station queues!\n");
                    exit(-1);
                }
                // Otherwise, queue it up
                s->queued_tu_present = 1;
                s->queued_tu = in_msg->source;

            } else {
                //Go ahead and let it come in now
                tw_event *e = tw_event_new(in_msg->source, 0, lp);
                message *msg = tw_event_data(e);
                // Station says its ok
                msg->type = ST_ACK;
                // All these passengers got on here I guess
                msg->source = self;
                tw_output(lp, "[%f] Sending ack message to %d!\n", tw_now(lp), in_msg->source);
                tw_event_send(e);
               
                s->curr_state = ST_OCCUPIED;
            }

            break;
        }
        case TRAIN_BOARD : {
            // Passengers have finished alighting, waiting passengers can board

            // TODO: Loop to send some boarding messages

            // All done boarding, go ahead and tell the train we are done
            tw_event *e = tw_event_new(in_msg->source, 0, lp);
            message *msg = tw_event_data(e);
            msg->type = P_COMPLETE;
            msg->source = self;
            tw_output(lp, "[%f] Sending boarding complete message to %d!\n", tw_now(lp), in_msg->source);
            tw_event_send(e);
        
            break;
        } 
        case TRAIN_DEPART : {
            tw_output(lp, "[%f] Train Departing %d\n", tw_now(lp), self);

            /*
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
            */
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
}

//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid station_map(tw_lpid gid){
    return (tw_peid) gid / g_tw_nlp;
}

