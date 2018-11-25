//The C main file for a ROSS model
//This file includes:
// - definition of the LP types
// - command line argument setup
// - a main function

//includes
#include "ross.h"
#include "passenger.h"
#include "model.h"

// Define LP types
//   these are the functions called by ROSS for each LP
//   multiple sets can be defined (for multiple LP types)
tw_lptype model_lps[] = {
  {
    (init_f) station_init,
    (pre_run_f) NULL,
    (event_f) station_event,
    (revent_f) station_event_reverse,
    (commit_f) NULL,
    (final_f) station_final,
    (map_f) station_map,
    sizeof(station_state)
  },
  {
    (init_f) transit_unit_init,
    (pre_run_f) NULL,
    (event_f) transit_unit_event,
    (revent_f) transit_unit_event_reverse,
    (commit_f) NULL,
    (final_f) transit_unit_final,
    (map_f) transit_unit_map,
    sizeof(tu_state)
  },
  { 0 },
};

//Define command line arguments default values
unsigned int station_count = 0;

// Global stuff
int g_num_stations = 0;
int g_num_transit_units = 0;

//add your command line opts
const tw_optdef model_opts[] = {
	TWOPT_GROUP("ROSS Model"),
	TWOPT_UINT("station_count", station_count, "Number of Stations"),
	TWOPT_END(),
};


//for doxygen
#define capacity_main main

int capacity_main (int argc, char* argv[]) {
	int i;
	int num_lps_per_pe;
    
    int total_nodes;

	tw_opt_add(model_opts);
	tw_init(&argc, &argv);

	//Do some error checking?
	//Print out some settings?

	//Custom Mapping
	/*
	g_tw_mapping = CUSTOM;
	g_tw_custom_initial_mapping = &model_custom_mapping;
	g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp;
	*/

	//Useful ROSS variables and functions
	// tw_nnodes() : number of nodes/processors defined
	// g_tw_mynode : my node/processor id (mpi rank)

	//Useful ROSS variables (set from command line)
	// g_tw_events_per_pe
	// g_tw_lookahead
	// g_tw_nlp
	// g_tw_nkp
	// g_tw_synchronization_protocol

	//Given our total number of PEs figure out how many LPs should go to each
    total_nodes = tw_nnodes();
    if ((station_count % total_nodes) != 0) {
        printf("Number of LPs must be divisible by nodes. (Stupid but ok)\n");
        return -1;
    }
	num_lps_per_pe = station_count / total_nodes;
    num_lps_per_pe += 1;

    // Set the two globals for the number of stations and stuff
    g_num_stations = station_count;
    g_num_transit_units = 1;

	//set up LPs within ROSS
	tw_define_lps(num_lps_per_pe, sizeof(message));
	// note that g_tw_nlp gets set here by tw_define_lps

	// IF there are multiple LP types
	//    you should define the mapping of GID -> lptype index
	g_tw_lp_typemap = &model_typemap;

	// set the global variable and initialize each LP's type
	g_tw_lp_types = model_lps;
	tw_lp_setup_types();

	// Do some file I/O here? on a per-node (not per-LP) basis

	tw_run();

	tw_end();

	return 0;
}
