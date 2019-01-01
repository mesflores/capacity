// station.c
//
// Functions for the Station LP

//Includes
#include <stdio.h>

#include "ross.h"
#include "passenger.h"
#include "model.h"
#include "graph_lib/graph.h"

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

    // Send yourself a message to kick of the passenger arrivals
    // TODO Pick a better inter-pass-arrival time
    if (self == 5) { 
        tw_event *e = tw_event_new(self, 3, lp);
        message *msg = tw_event_data(e);
        msg->type = P_ARRIVE;
        // All these passengers got on here I guess
        msg->source= self;
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
        case P_ARRIVE : {
            //  A new passenger has arrived at the station
            // Pick a dest.
            // TODO: Pick these for real...
            // For now, send them to the last stop
            int dest = 9;

            tw_output(lp, "[%f] Passenger arriving at %d!\n", tw_now(lp), self);
            passenger_t* new_pass = create_passenger(self, dest, tw_now(lp));

            // Put him in the list
            new_pass->next = s->pass_list;
            s->pass_list = new_pass;

            // Sechedule the next arrival
            // TODO Pick a better inter-pass-arrival time
            tw_event *e = tw_event_new(self, 3, lp);
            message *msg = tw_event_data(e);
            msg->type = P_ARRIVE;
            // All these passengers got on here I guess
            msg->source = self;
            // TODO: reactivate
            //tw_event_send(e);

            break;
        }
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
                tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
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

            passenger_t* prev_pass = NULL;
            passenger_t* curr_pass = s->pass_list;
   
            // Ok we have somebody to check
            // XXX XXX XXX I think this might be very hard to do backwards... XXX XXX XXX
            if (curr_pass != NULL) {
                while(curr_pass != NULL) {
                    if (should_board(curr_pass) == 1) {
                        // Send  boarding message!
                        tw_output(lp, "[%f] Sending boarding message to %d!\n", tw_now(lp), in_msg->source);
                        tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
                        message *msg = tw_event_data(e);
                        msg->type = P_BOARD;
                        msg->source = self;
                        // Just copy the passenger on
                        memcpy(&(msg->curr_pass), curr_pass, sizeof(passenger_t));
                        // Clear out the list in the copy
                        (msg->curr_pass).next=NULL;
                        tw_event_send(e);

                        // Cut the curr pass out of the list
                        if (prev_pass == NULL) {
                            s->pass_list = curr_pass->next;
                        } else {
                            prev_pass->next = curr_pass->next;
                        }

                        // Hard break out
                        return;
                    }
                    // Advance the loop 
                    prev_pass = curr_pass;
                    curr_pass = curr_pass->next;
                }
            }
            // Ok so if we made it here, either: no passengers or the ones that are here shouldnt board


            // All done boarding, go ahead and tell the train we are done
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
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

