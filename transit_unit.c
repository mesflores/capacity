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
    return;
}

//Forward event handler
void transit_unit_event (tu_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
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

