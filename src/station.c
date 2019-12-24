// station.c
//
// Functions for the Station LP

//Includes
#include <stdio.h>
#include <string.h>

#include "ross.h"
#include "passenger.h"
#include "model.h"
#include "graph.h"
#include "track.h"

//Helper Functions
void SWAP (int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
void SWAP_SHORT (short *a, short *b) {
    short tmp = *a;
    *a = *b;
    *b = tmp;
}




//Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void station_init (station_state *s, tw_lp *lp) {
    int self = lp->gid;

    // init state data
    s->left.track_id = 0;
    s->left.inbound = ST_EMPTY; 
    s->left.outbound = ST_EMPTY; 
    s->left.curr_tu = 0;
    s->left.queued_tu_present = 0;
    memset(s->left.queued_tu, 0, sizeof(s->left.queued_tu));
    s->left.next_arrival = 0;
    s->left.from_queue = 0;

    s->right.track_id = 1;
    s->right.inbound = ST_EMPTY; 
    s->right.outbound = ST_EMPTY; 
    s->right.curr_tu = 0;
    s->right.queued_tu_present = 0;
    memset(s->right.queued_tu, 0, sizeof(s->right.queued_tu));
    s->right.next_arrival = 0;
    s->right.from_queue = 0;

    s->pass_list = 0;

    // Lookup the name
    memset(s->station_name, 0, 25); //TODO: Fix this size
    sta_name_copy(s->station_name, self);

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

    track_t* curr_track;

    // initialize the bit field
    // TODO: Understand what these are for...
    *(int *) bf = (int) 0;

    // update the current state
    // however, save the old value in the 'reverse' message
    // SWAP(&(s->last_arr), &(in_msg->passenger_count));

    // Look up what track the message came from
    curr_track = track_map(self, in_msg->prev_station, s, in_msg);

    // handle the message
    switch (in_msg->type) {
        case TRAIN_ARRIVE : {
            tw_output(lp, "\n[%.3f] ST %d: Train %d arriving at %s on track %d!\n", tw_now(lp), self, in_msg->source, sta_name_lookup(self), curr_track->track_id);
                 
            fprintf(node_out_file, "[ST %d]: Train %lu arriving at %s on track %d!\n", self, in_msg->source, sta_name_lookup(self), curr_track->track_id);
            fflush(node_out_file);
            
            // First, check to see what our state is
            if ((curr_track->inbound == ST_OCCUPIED) || (curr_track->inbound == ST_BOARDING)) {
                // If the current track is occupied by us, well that's 
                // bad, go ahead and suspend
                if (curr_track->queued_tu[0] == in_msg->source){
                    fprintf(node_out_file, "[ST %d]: Spurious TRAIN_ARRIVE from %ld\n", self, in_msg->source);
                    fflush(node_out_file);
                    tw_lp_suspend(lp, 0, 0);
                    return;
                }

                // If we are currently occupied, put the TU in the queue for 
                // notification when the state transitions to empty
                add_train(in_msg->source, curr_track); 

                tw_output(lp, "\n[%.3f] ST: %d: Queuening up train %d\n", tw_now(lp), self, in_msg->source); 
                fprintf(node_out_file, "[ST %d]: Queueing up %lu\n", self, in_msg->source);
                fprintf(node_out_file, "[ST %d]: Already here: %lu\n", self, curr_track->queued_tu[0]);
                fflush(node_out_file);

            } else {
                //Go ahead and let it come in now
                tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
                message *msg = tw_event_data(e);
                // Station says its ok
                msg->type = ST_ACK;
                // All these passengers got on here I guess
                msg->source = self;
                msg->next_arrival = curr_track->next_arrival;
                //tw_output(lp, "[%.3f] ST %d: Sending ack message to %d on track %d!\n", tw_now(lp), self, in_msg->source, curr_track->track_id);
                //fprintf(node_out_file, "[ST %d]: Sending ack message to %lu on track %d!\n", self, in_msg->source, curr_track->track_id);
                tw_event_send(e);
               
                curr_track->inbound = ST_OCCUPIED;
                curr_track->curr_tu = in_msg->source;
            }

            break;
        }
        case P_ARRIVE : {
            fprintf(node_out_file, "[ST %d]: P_Arrive from %lu\n", self, in_msg->source);
            fflush(node_out_file);
            /******* Disabled ******
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
            ************************/
            break;
        }
        case TRAIN_BOARD : {
            // Passengers have finished alighting, waiting passengers can board
            // TODO: Loop to send some boarding messages
            fprintf(node_out_file, "[ST %d]: Received TRAIN_BOARD from %ld\n", self, in_msg->source);
            fflush(node_out_file);

            // First check that we are in the right state
            if (curr_track->inbound != ST_OCCUPIED) {
                // It was in a weird state, suspend
                fprintf(node_out_file, "[ST %d]: TRAIN_BOARD but I wasnt in occuppied from %ld - suspend\n", self, in_msg->source);
                fflush(node_out_file);
                tw_lp_suspend(lp, 0, 0);
                return;
            }

            // Was this train actualy in the station?
            if (curr_track->curr_tu != in_msg->source) {
                // We got a board from someone that shouldnt have received the ack yet
                fprintf(node_out_file, "[ST %d]: Spurious TRAIN_BOARD from %ld (not in station)\n", self, in_msg->source);
                fflush(node_out_file);
                tw_lp_suspend(lp, 0, 0);
                return;
            }

            // Set mode to boarding
            curr_track->inbound = ST_BOARDING;

            passenger_t* prev_pass = NULL;
            passenger_t* curr_pass = s->pass_list;

#if 0
            // Ok we have somebody to check
            // XXX XXX XXX I think this might be very hard to do backwards... XXX XXX XXX
            if (curr_pass != NULL) {
                while(curr_pass != NULL) {
                    if (should_board(curr_pass) == 1) {
                        // Send  boarding message!
                        //tw_output(lp, "[%.3f] ST %d: Sending boarding message to %d!\n", tw_now(lp), self, in_msg->source);
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
#endif
            // Ok so if we made it here, either: no passengers or the ones that are here shouldnt board


            // All done boarding, go ahead and tell the train we are done
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = P_COMPLETE;
            msg->source = self;
            //fprintf(node_out_file, "[ST %d]: Sending P_COMPLETE to %ld\n", self, in_msg->source);
            //tw_output(lp, "[%.3f] ST %d: Sending boarding complete message to %d on track %d!\n", tw_now(lp), self, in_msg->source, curr_track->track_id);
            tw_event_send(e);
        
            break;
        } 
        case TRAIN_DEPART : {
            // Are we in the right state for this?
            if (curr_track->inbound != ST_BOARDING) {
                // We got a depart from someone that shouldnt have received the ack yet
                fprintf(node_out_file, "[ST %d]: Spurious TRAIN_DEPART (wrong state) from %ld\n", self, in_msg->source);
                fflush(node_out_file);
                tw_lp_suspend(lp, 0, 0);
                return;
            }

            // Was this train actualy in the station?
            if (curr_track->curr_tu != in_msg->source) {
                // We got a depart from someone that shouldnt have received the ack yet
                fprintf(node_out_file, "[ST %d]: Spurious TRAIN_DEPART from %ld\n", self, in_msg->source);
                fflush(node_out_file);
                tw_lp_suspend(lp, 0, 0);
                return;
            }



            tw_lpid curr_tu = 0;
            //tw_output(lp, "[%.3f] ST %d: Train %d Departing %s on track %d\n", tw_now(lp), self, in_msg->source, sta_name_lookup(self), curr_track->track_id);
            fprintf(node_out_file, "[ST %d]: Train %lu Departing %s on track %d!\n", self, in_msg->source, sta_name_lookup(self), curr_track->track_id);
            fflush(node_out_file);
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

            // Log the next arrival time
            //curr_track->next_arrival = in_msg->next_arrival;
            SWAP(&(curr_track->next_arrival), &(in_msg->next_arrival));

            //stash the current from_queue status
            SWAP_SHORT(&(curr_track->from_queue), &(in_msg->from_queue));

            // Go ahead and ack a queued train if there is one
            if (curr_track->queued_tu_present > 0) {
                // Pop a TU off the queue
                curr_tu = pop_head(curr_track);

                tw_event *e = tw_event_new(curr_tu, CONTROL_EPOCH, lp);
                message *msg = tw_event_data(e);
                // Station says its ok
                msg->type = ST_ACK;
                // All these passengers got on here I guess
                msg->source = self;
                //tw_output(lp, "[%.3f] ST %d: Sending ack message to queued TU %d!\n", tw_now(lp), self, curr_tu);
                //tw_output(lp, "[%.3f] ST %d: Sending ack message to queued TU %d!\n", tw_now(lp), self, curr_track->queued_tu);
                tw_event_send(e);
              
                // Bump to occupied, continue
                curr_track->inbound = ST_OCCUPIED;
                curr_track->curr_tu = curr_tu;

                // Label it as being from the queue
                curr_track->from_queue = 1;
            } else {
                // Nothing waiting, just go back to empty
                curr_track->inbound = ST_EMPTY;
                curr_track->curr_tu = 0;
                curr_track->from_queue = 0;
            }

            break;


        }
        default :
            fprintf(node_out_file, "Station Unhandeled forward message type %d\n", in_msg->type);
    }

}


//Reverse Event Handler
void station_event_reverse (station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;

    track_t* curr_track;
    curr_track = track_map_rev(self, in_msg->prev_station, s, in_msg);

    switch (in_msg->type) {
        case P_ARRIVE : {
            // TODO: doesn't do anything right now, so the reverse is easy?
            //fprintf(node_out_file, "[ST %d]: STA reverse P_ARRIVE call from %lu !\n", self, in_msg->source);
            break;
        }
        case TRAIN_ARRIVE : {
            fprintf(node_out_file, "[ST %d]: STA reverse TRAIN_ARRIVE call from %lu at track %d!\n", self, in_msg->source, curr_track->track_id);
            // If we had something queued, that means this arrival queued, clear it 
            if (curr_track->queued_tu_present > 0) {
                fprintf(node_out_file, "[ST %d]: other trains in the station!\n", self);
                fprintf(node_out_file, "[ST %d]: In my seat: %lu !\n", self, curr_track->queued_tu[0]);
                pop_tail(curr_track);   
                //curr_track->queued_tu_present = 0; // In a better world this would decrement
                //(curr_track->queued_tu)[0] = 0;
            } else {
                // This train was in the station
                fprintf(node_out_file, "[ST %d]: It was just me, empty now!!\n", self);
                curr_track->inbound = ST_EMPTY;
                curr_track->curr_tu = 0;
            }

            fflush(node_out_file);
            break;
        }
        case TRAIN_BOARD : {
            fprintf(node_out_file, "[ST %d]: STA reverse TRAIN_BOARD call from %lu!\n", self, in_msg->source);
            fflush(node_out_file);
            // TODO: Actually this is incomplete for now, since receiving
            // a train_board does nothing but generate a P_COMPLETE
            curr_track->inbound = ST_OCCUPIED;
            break;
        }
        case TRAIN_DEPART : {
            fprintf(node_out_file, "[ST %d]: STA reverse TRAIN_DEPART call from %lu!\n", self, in_msg->source);
            fflush(node_out_file);
            // No matter what, mark track as back in boarding
            curr_track->inbound = ST_BOARDING;
            curr_track->curr_tu = in_msg->source;
            // Was that train ever queued?
            // If so, we need to put them back in the queue, otherwise, nothing
            if (curr_track->from_queue == 1) {
                // Put the one thats on the track now back
                add_train_head(curr_track->curr_tu, curr_track);
            }

            // reset 'from_queue' back to what it was
            SWAP_SHORT(&(curr_track->from_queue), &(in_msg->from_queue));

            // Reset next_arrival
            SWAP(&(curr_track->next_arrival), &(in_msg->next_arrival));

            break;
        }

        default :
            fprintf(node_out_file, "Station Unhandled reverse message type %d\n", in_msg->type);
    }
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

