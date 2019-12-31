// model_mappings.h

#include "ross.h"
#include "passenger.h"
#include "model.h"

// Multiple LP Types mapping function
//    Given an LP's GID
//    Return the index in the LP type array (defined in model_main.c)
tw_lpid model_typemap (tw_lpid gid) {
    // the girst g_num_stations are stations and the remaining are transit_units
    if (gid < g_num_stations) {
        return STATION;
    }
    else {
        return TRANSIT_UNIT;
    }
}

//Custom mapping functions are used so
// - no LPs are unused
// - event activity is balanced
extern unsigned int nkp_per_pe;
//#define VERIFY_MAPPING 1 //useful for debugging
//This function maps LPs to KPs on PEs and is called at the start
//This example is the same as Linear Mapping
void model_custom_mapping_linear(void){
    unsigned int nlp_per_kp;
    tw_lpid  lpid;
    tw_kpid  kpid;
    unsigned int j;

    // may end up wasting last KP, but guaranteed each KP has == nLPs
    nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);

    if(!nlp_per_kp) {
        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
    }

    g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

#if VERIFY_MAPPING
    fprintf(node_out_file, "NODE %lu: nlp %lu, offset %lu\n", g_tw_mynode, g_tw_nlp, g_tw_lp_offset);
    fprintf(node_out_file, "\tPE %lu\n", g_tw_pe->id);
#endif

    for(kpid = 0, lpid = 0; kpid < nkp_per_pe; kpid++) {
        tw_kp_onpe(kpid, g_tw_pe);

#if VERIFY_MAPPING
        fprintf(node_out_file, "\t\tKP %lu", kpid);
#endif

        for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
            tw_lp_onpe(lpid, g_tw_pe, g_tw_lp_offset+lpid);
            tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]);

#if VERIFY_MAPPING
            if(0 == j % 20) {
                fprintf(node_out_file, "\n\t\t\t");
            }
            fprintf(node_out_file, "%lu ", lpid+g_tw_lp_offset);
#endif
        }

#if VERIFY_MAPPING
        fprintf(node_out_file, "\n");
#endif
    }

    if(!g_tw_lp[g_tw_nlp-1]) {
        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
    }

    if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1) {
        tw_error(TW_LOC, "LPs not sequentially enumerated!");
    }
}

void model_custom_mapping_rr(void){
    unsigned int nlp_per_kp;
    tw_lpid  lpid;
    tw_kpid  kpid;
    unsigned int i, j;

    int lp_stride = tw_nnodes();
    int lp_offset = g_tw_mynode;

    tw_lpid curr_pe;

    g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

    // may end up wasting last KP, but guaranteed each KP has == nLPs
    nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);


    if(!nlp_per_kp) {
        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
    }

#if VERIFY_MAPPING
    fprintf(node_out_file, "NODE %lu: nlp %lu\n", g_tw_mynode, g_tw_nlp);
    fprintf(node_out_file, "\tPE %lu\n", g_tw_pe->id);
    fflush(node_out_file);
#endif

    // Do all the KPs first
    for(i = 0; i < nkp_per_pe; i++) {
        fprintf(node_out_file, "\tKP %d\n", i);
        tw_kp_onpe(i, g_tw_pe);
    }

    // ASSUMPTION: g_tw_nlp is the same on each rank
    for(j = 0, lpid=lp_offset; j < g_tw_nlp; j++, lpid+=lp_stride) {
        kpid = j % nkp_per_pe;
        tw_lp_onpe(j, g_tw_pe, lpid);
        tw_lp_onkp(g_tw_lp[j], g_tw_kp[kpid]);

#if VERIFY_MAPPING
        fprintf(node_out_file, "Local %4u LP %4lu KP %4lu PE %4lu\n", j, lpid, kpid, g_tw_pe->id);
        fflush(node_out_file);
#endif
    }


#if VERIFY_MAPPING
    int local_id;
    // Loop over every ID and look it up forward and reverse
    //for(i = 0; i < ng_tw_nlp * tw_nnodes(); i++) {
    for (j = g_tw_mynode; j <  g_tw_nlp * tw_nnodes(); j+=lp_stride) {
        local_id = j / tw_nnodes();  
        fprintf(node_out_file, "Map %d to %d\n", j, local_id);
        if (j != g_tw_lp[local_id]->gid) {
            tw_error(TW_LOC, "Mapping Inconsistency at %d", j);
        } 
    }
#endif

    if(!g_tw_lp[g_tw_nlp-1]) {
        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
    }

}

//Given a gid, return the local LP (global id => local id mapping)
tw_lp * model_mapping_to_lp(tw_lpid lpid){
    int local_id = lpid - g_tw_lp_offset;
    return g_tw_lp[local_id];
}

//Given a gid, return the local LP (global id => local id mapping)
tw_lp * model_mapping_to_lp_rr(tw_lpid lpid){
    int local_id = lpid / tw_nnodes();
    return g_tw_lp[local_id];
}

