// transit_unit.c
//
// Functions for the transit_unit LP

//Includes
#include <stdio.h>

#include "ross.h"
#include "passenger.h"
#include "model.h"
#include "graph.h"
#include "route.h"

//Helper Functions
void SWAP_UL (unsigned long *a, unsigned long *b) {
    unsigned long tmp = *a;
    *a = *b;
    *b = tmp;
}

//Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void transit_unit_init (tu_state *s, tw_lp *lp) {
    int self = lp->gid;

    // Do we want to init this even?
    if (self >= (g_num_stations + g_num_transit_units)) {
        // It's not a real train, do nothing
        //
        // Not sure if this is the best way to handle this, but basically,
        // since we need an equal number of LPs per PE, this just fills it out.
        return;
    }


    // Init the train state
    // TODO: At some point schedule initialization will be a complicated question
    s->curr_state = TU_IDLE; // Starts nowhere

    s->prev_station = -1;
    s->station = -1; // XXX I dunno, we'll figure it out XXX
    s->route_index = 0;

    // Go ahead and init the route
    abstract_route_t* my_route = get_route(self);
    s->route = init_route(my_route->stops, my_route->length);
    s->start = my_route->start_time - g_time_offset;
    s->pass_list = NULL;
    s->pass_count = 0;

    return;
}

//Pre-run handler
void transit_unit_pre_run (tu_state *s, tw_lp *lp) {
    int self = lp->gid;

    // Need to do this again, or the pre run handler will eat it
    if (self >= (g_num_stations + g_num_transit_units)) {
        // Not sure if this is the best way to handle this, but basically,
        // since we need an equal number of LPs per PE, this just fills it out.
        return;
    }


    // Send an approach message to the first station on the schedule
    tw_event *e = tw_event_new(s->route->origin, s->start, lp);
    message *msg = tw_event_data(e);
    msg->type = TRAIN_ARRIVE;
    // All these passengers got on here I guess
    msg->source = self;
    msg->prev_station = s->station;
    tw_output(lp, "[%.3f] TU %d: Sending arrive message to %s\n", tw_now(lp), self, sta_name_lookup(s->route->origin));
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
    char curr_station;


    // initialize the bit field
    // TODO: Understand what these are for...
    *(int *) bf = (int) 0;

    // update the current state
    // however, save the old value in the 'reverse' message
    // SWAP(&(s->last_arr), &(in_msg->passenger_count));

    // handle the message
    switch (in_msg->type) {
        case ST_ACK : {
            tw_output(lp, "[%.3f] TU %d: ACK received from %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
            // A station said we are good to go!
            // Go ahead and put yourself in alight mode 
            s->curr_state = TU_ALIGHT; 
            // Save the previous station
            //s->prev_station = s->station;
            SWAP_UL(&(s->prev_station), &(s->station));
            //s->station = in_msg->source;
            SWAP_UL(&(s->station), &(in_msg->source));
            

            //s->min_time = in_msg->next_arrival;
            SWAP(&(s->min_time), &(in_msg->next_arrival));

            // TODO: Send messages to station to empty passengers

            // For now, no passengers go right to boarding
            tw_event *e = tw_event_new(s->station, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_BOARD;
            msg->prev_station = s->prev_station;
            msg->source = self;
            tw_output(lp, "[%.3f] TU %d: Sending alighting complete to %s!\n", tw_now(lp), self, sta_name_lookup(s->station));
            tw_event_send(e);

            // Train is now accepting boarding passengers
            s->curr_state = TU_BOARD;

            break;
        }
        case P_COMPLETE : {
            // station is all done boarding 

            s->curr_state = TU_APPROACH;

            // Ok tell the next station that we are on our way 
            next_station = get_next(s->route, &(s->route_index));

            // Bump the routeindex
            s->route_index += 1;

            if (next_station != -1) {
                // Time it takes to get to the next station
                delay = get_delay_id(in_msg->source, next_station);
                // Actually its possible somebody ahead of us was delayed, check the min time
                if (delay < (s->min_time - tw_now(lp))) {
                    delay = s->min_time - tw_now(lp) + 1; // TODO: Controllable
                    tw_output(lp, "[%.3f] TU %d: Incurred cascading delay!\n", tw_now(lp), self);
                }
            }

            // Ok all done go ahead and depart
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_DEPART;
            msg->source = self;
            msg->prev_station = s->prev_station;
            // If we arent going anywhere just 0 this
            if (next_station == -1) {
                msg->next_arrival = 0;
            }
            else {
                msg->next_arrival = tw_now(lp) + delay;
            }
            tw_output(lp, "[%.3f] TU %d: Sending depart to %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
            tw_event_send(e);

            // Actually we were at the end, so just bail
            if (next_station == -1) {
                // TODO: is there a better way to terminate?
                // We do need to tell the last station that we left
                tw_output(lp, "[%.3f] TU %d:Route ending at STA %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
                break;
            }

            // Not at that station anymore
            // We actually want to use this, not going to zero it out yet
            //s->station = -1;

            // Send it an approach message
            tw_event *approach = tw_event_new(next_station, delay, lp);
            message *next_msg = tw_event_data(approach);
            next_msg->type = TRAIN_ARRIVE;
            next_msg->source = self;
            next_msg->prev_station = s->station;
            tw_output(lp, "[%.3f] TU %d: Sending approach to %s!\n", tw_now(lp), self, sta_name_lookup(next_station));
            tw_event_send(approach);
            break;
        }
        case P_BOARD : {
            /****** XXX This should be disabled, as passengers are not generated atm *****/
            // Passenger!
            passenger_t* new_pass = tw_calloc(TW_LOC, "create_passenger", sizeof(passenger_t), 1);
            // Copy it in
            memcpy(new_pass, &(in_msg->curr_pass), sizeof(passenger_t));
            // Stick it in the list
            new_pass->next = s->pass_list;
            s->pass_list = new_pass;
            s->pass_count += 1;
            tw_output(lp, "[%.3f] TU %d: Passenger boarded train %d!\n", tw_now(lp), self, self);

            // Have it continue boarding.
            // TODO: Probably we could do this optimistically instead of explicit acks...
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_BOARD;
            msg->source = self;
            msg->prev_station = s->prev_station;
            tw_output(lp, "[%.3f] TU %d: Continue boarding to %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
            tw_event_send(e);
            break;
        }
        default :
            printf("TU %d: Unhandeled forward message type %d\n", self, in_msg->type);
    }


    return;
}


//Reverse Event Handler
void transit_unit_event_reverse (tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
    int self = lp->gid;
    switch (in_msg->type) {
        case ST_ACK : {
            //printf("TU reverse ST_ACK call for TU %d!\n", self);
            //reset to approach
            s->curr_state = TU_APPROACH;

            // Reset the previous to whatever it was
            SWAP_UL(&(s->station), &(in_msg->source));
            // Reset the curr to the previous
            SWAP_UL(&(s->prev_station), &(s->station));

            //Reset the min time
            SWAP(&(s->min_time), &(in_msg->next_arrival));

            break;
        }
        case P_COMPLETE : {
            //printf("TU reverse P_COMPLETE call!\n");
            s->route_index -= 1;
            s->curr_state = TU_BOARD;
            break;
        }
        case P_BOARD : {
            //printf("TU reverse P_BOARD call!\n");
            // TODO: Currently disabled!
            break;
        }
        default :
            printf("TU %d: Unhandeled reverse message type %d\n", self, in_msg->type);
    }
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

