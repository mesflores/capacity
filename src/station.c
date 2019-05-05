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



// Map to a left or right track
track_t* track_map(int curr_station, int prev_station, station_state* s) {
    // Left
    if (prev_station < curr_station) {
        return &(s->left); 
        // Right
    } else if (prev_station > curr_station) {
        return &(s->right);
        // This is basically an assert...
    } else {
        printf("Invalid matching stations: curr: %d prev: %d\n", curr_station, prev_station);
        exit(-1);
    }
}

// Queue management 
int add_train(tw_lpid new_train, track_t* track) {
    // Add a new train -- find the first nonzero spot
    // if queued_tu_present is 0, add to 0
    // if 1, it means 1 train already here, add to 1
    int index = track->queued_tu_present;

    // Sanity check that it hasn't filled up
    if (index - 1  > QUEUE_LEN) {
        printf("Station queue exceeded!\n");
        exit(-1);
    }

    // Add it in
    track->queued_tu[index] = new_train;
    track->queued_tu_present += 1;

    /// Return where we put it
    return index;
}
int pop_head(track_t* track) {
    // Grab the guy at the head
    tw_lpid curr_train = track->queued_tu[0];

    // Scoot everybody down
    for (int i=0; i < (QUEUE_LEN - 1); i++) {
        track->queued_tu[i] = track->queued_tu[i+1];
    }
    
    // Decrease the queued count
    track->queued_tu_present -= 1;

    // Return that guy we got from the front
    return curr_train;
}
int add_train_head(tw_lpid new_train, track_t* track) {
    // Here we need to add somebody back to the head

    // First, scoot everyone else down
    for (int i=1; i < (QUEUE_LEN - 1); i++) {
        track->queued_tu[i] = track->queued_tu[i+1];
    }

    // Put the new one in the right spot
    track->queued_tu[0] = new_train;
    // Bump up the count
    track->queued_tu_present += 1;

    return 0;
}
int pop_tail(track_t* track) {
    // Just chop the guy off the back

    int curr_index = track->queued_tu_present - 1;
    tw_lpid curr_tail = track->queued_tu[curr_index]; 

    // Clear out that tail
    track->queued_tu[curr_index] = 0;
    // decrement the number present
    track->queued_tu_present -= 1;

    return curr_tail;
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
    curr_track = track_map(self, in_msg->prev_station, s);

    // handle the message
    switch (in_msg->type) {
        case P_ARRIVE : {
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
        case TRAIN_ARRIVE : {
            tw_output(lp, "[%.3f] ST %d: Train %d arriving at %s!\n", tw_now(lp), self, in_msg->source, sta_name_lookup(self));
          
            // First, check to see what our state is
            if ((curr_track->inbound == ST_OCCUPIED) || (curr_track->inbound == ST_BOARDING)) {
                // If we are currently occupied, put the TU in the queue for 
                // notification when the state transitions to empty
                add_train(in_msg->source, curr_track); 

                /*
                // If we are currently occupied, put the TU in the queue for 
                // notification when the state transitions to empty
                // Was anybody queued? if so hard error
                if (curr_track->queued_tu_present > 0) {
                    printf("[%.3f] NOT IMPLEMENTED: proper station queues! ST: %d TU: %lu\n", tw_now(lp), self, in_msg->source);
                    printf("[%.3f] Currently in the track %lu\n", tw_now(lp), curr_track->curr_tu);
                    printf("[%.3f] Currently in the queue %lu\n", tw_now(lp), (curr_track->queued_tu)[0]);
                    exit(-1);
                }
                // Otherwise, queue it up
                curr_track->queued_tu_present = 1;
                (curr_track->queued_tu)[0] = in_msg->source;
                */
                tw_output(lp, "[%.3f] ST: %d: Queuening up train %d\n", tw_now(lp), self, in_msg->source); 

            } else {
                //Go ahead and let it come in now
                tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
                message *msg = tw_event_data(e);
                // Station says its ok
                msg->type = ST_ACK;
                // All these passengers got on here I guess
                msg->source = self;
                msg->next_arrival = curr_track->next_arrival;
                tw_output(lp, "[%.3f] ST %d: Sending ack message to %d on track %d!\n", tw_now(lp), self, in_msg->source, curr_track->track_id);
                tw_event_send(e);
               
                curr_track->inbound = ST_OCCUPIED;
                curr_track->curr_tu = in_msg->source;
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
                        tw_output(lp, "[%.3f] ST %d: Sending boarding message to %d!\n", tw_now(lp), self, in_msg->source);
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
            tw_output(lp, "[%.3f] ST %d: Sending boarding complete message to %d!\n", tw_now(lp), self, in_msg->source);
            tw_event_send(e);
        
            break;
        } 
        case TRAIN_DEPART : {
            tw_lpid curr_tu = 0;
            tw_output(lp, "[%.3f] ST %d: Train %d Departing %s\n", tw_now(lp), self, in_msg->source, sta_name_lookup(self));
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
                curr_tu = pop_head(curr_track);
                tw_event *e = tw_event_new(curr_tu, CONTROL_EPOCH, lp);
                //tw_event *e = tw_event_new((curr_track->queued_tu)[0], CONTROL_EPOCH, lp);
                message *msg = tw_event_data(e);
                // Station says its ok
                msg->type = ST_ACK;
                // All these passengers got on here I guess
                msg->source = self;
                tw_output(lp, "[%.3f] ST %d: Sending ack message to queued TU %d!\n", tw_now(lp), self, curr_tu);
                //tw_output(lp, "[%.3f] ST %d: Sending ack message to queued TU %d!\n", tw_now(lp), self, curr_track->queued_tu);
                tw_event_send(e);
              
                // Bump to occupied, continue
                curr_track->inbound = ST_OCCUPIED;
                curr_track->curr_tu = curr_tu;
                //curr_track->curr_tu = (curr_track->queued_tu)[0];

                // Don't need this, since queue functions manage
                // Clear out the queue
                //curr_track->queued_tu_present = 0;
                //(curr_track->queued_tu)[0] = 0;

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
            printf("Station Unhandeled forward message type %d\n", in_msg->type);
    }

}


//Reverse Event Handler
void station_event_reverse (station_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;

    track_t* curr_track;

    // Look up what track the message came from
    // Is this ok
    curr_track = track_map(self, in_msg->prev_station, s);
    
    switch (in_msg->type) {
        case P_ARRIVE : {
            // TODO: doesn't do anything right now, so the reverse is easy?
            //printf("STA reverse P_ARRIVE call!\n");
            break;
        }
        case TRAIN_ARRIVE : {
            //printf("STA reverse TRAIN_ARRIVE call!\n");
            // If we had something queued, that means this arrival queued, clear it 
            if (curr_track->queued_tu_present > 0) {
                pop_tail(curr_track);   
                //curr_track->queued_tu_present = 0; // In a better world this would decrement
                //(curr_track->queued_tu)[0] = 0;
            } else {
                // This train was in the station
                curr_track->inbound = ST_EMPTY;
                curr_track->curr_tu = 0;
            }

            break;
        }
        case TRAIN_BOARD : {
            //printf("STA reverse TRAIN_BOARD call!\n");
            // TODO: Actually this is blank for now, since receiving
            // a train_board does nothing but generate a P_COMPLETE
            break;
        }
        case TRAIN_DEPART : {
            //printf("STA reverse TRAIN_DEPART call!\n");
            // No matter what, mark track as occupied
            curr_track->inbound = ST_OCCUPIED;
            curr_track->curr_tu = in_msg->source;
            // Was that train every queued?
            // If so, we need to put them back in the queue, otherwise, nothing
            if (curr_track->from_queue == 1) {
                // Put the one thats on the track now back
                add_train_head(curr_track->curr_tu, curr_track);
                //curr_track->queued_tu_present = 1;
                //(curr_track->queued_tu)[0] = curr_track->curr_tu;
                 

            }

            // TODO XXX: we actually need to reset 'from_queue' back to what it was
            SWAP_SHORT(&(curr_track->from_queue), &(in_msg->from_queue));

            // Reset next_arrival
            SWAP(&(curr_track->next_arrival), &(in_msg->next_arrival));

            break;
        }

        default :
            printf("Station Unhandled reverse message type %d\n", in_msg->type);
    }
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

