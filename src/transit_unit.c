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


//Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void transit_unit_init (tu_state *s, tw_lp *lp) {
    int self = lp->gid;

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
            // A station said we are good to go!
            // Go ahead and put yourself in alight mode 
            s->curr_state = TU_ALIGHT; 
            // Save the previous station
            s->prev_station = s->station;
            s->station = in_msg->source;

            // TODO: Send messages to station to empty passengers

            // For now, no passengers go right to boarding
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_BOARD;
            msg->prev_station = s->prev_station;
            msg->source = self;
            //tw_output(lp, "[%.3f] TU %d: Sending alighting complete to %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
            tw_event_send(e);

            // Train is now accepting boarding passengers
            s->curr_state = TU_BOARD;

            break;
        }
        case P_COMPLETE : {
            // station is all done boarding 
            // Ok all done go ahead and depart
            tw_event *e = tw_event_new(in_msg->source, CONTROL_EPOCH, lp);
            message *msg = tw_event_data(e);
            msg->type = TRAIN_DEPART;
            msg->source = self;
            msg->prev_station = s->prev_station;
            tw_output(lp, "[%.3f] TU %d: Sending depart to %s!\n", tw_now(lp), self, sta_name_lookup(in_msg->source));
            tw_event_send(e);

            // Not at that station anymore
            // We actually want to use this, not going to zero it out yet
            //s->station = -1;

            // Ok tell the next station that we are on our way 
            next_station = get_next(s->route, &(s->route_index));
            // Actually we were at the end
            if (next_station == -1) {
                // TODO: is there a better way to terminate?
                break;
            }
            // Bump the routeindex
            s->route_index += 1;

            // Time it takes to get to the next station
            delay = get_delay_id(in_msg->source, next_station);

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

