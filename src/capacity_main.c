//The C main file for a ROSS model
//This file includes:
// - definition of the LP types
// - command line argument setup
// - a main function

//includes
#include "ross.h"
#include "graph.h"
#include "route.h"
#include "passenger.h"
#include "model.h"
#include "station.h"
#include "transit_unit.h"

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
    (map_f) lp_map_rr,
    sizeof(station_state)
  },
  {
    (init_f) transit_unit_init,
    (pre_run_f) transit_unit_pre_run,
    (event_f) transit_unit_event,
    (revent_f) transit_unit_event_reverse,
    (commit_f) NULL,
    (final_f) transit_unit_final,
    (map_f) lp_map_rr,
    sizeof(tu_state)
  },
  { 0 },
};

// Define the tracing functions
st_model_types model_types[] = {
  {
    (ev_trace_f) station_ev_trace,
    0, // Size
    (model_stat_f) station_stat_collect,
    0, // size
    NULL, //(sample_event_f)
    NULL, //(sample_revent_f)
    0
  },
  {
    (ev_trace_f) transit_unit_ev_trace,
    0, // Size
    (model_stat_f) transit_unit_stat_collect,
    0, // size
    NULL, //(sample_event_f)
    NULL, //(sample_revent_f)
    0
  },
  { 0 },
};

// Global stuff
int g_num_stations = 10;
int g_num_transit_units = 0;
int g_time_offset = 0;
FILE * node_out_file;

//Intermediate GTFS data
char g_adj_mat_fn[1024] = "dat/mat.dat";
char g_routes_fn[1024] = "dat/routes.dat";

//add your command line opts
const tw_optdef model_opts[] = {
    TWOPT_GROUP("Capacity Model"),
    TWOPT_CHAR("mat", g_adj_mat_fn, "Adjacency matrix file"),
    TWOPT_CHAR("routes", g_routes_fn, "Routes file"),
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

    // Create files for debugging
    char debugfilename[100];
#if DEBUG_FILE_OUTPUT
    sprintf(debugfilename, "out_dat/node_%ld_output_file.txt", g_tw_mynode);
    node_out_file = fopen(debugfilename, "w");
    fprintf(node_out_file, "PID: %d\n", getpid());
#endif

    // Init the global vars
    graph_init(g_adj_mat_fn);
    init_global_routes(g_routes_fn);

    //Custom Mapping
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &model_custom_mapping_rr;
    g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp_rr;

    //Useful ROSS variables and functions
    // tw_nnodes() : number of nodes/processors defined
    // g_tw_mynode : my node/processor id (mpi rank)

    //Useful ROSS variables (set from command line)
    // g_tw_events_per_pe
    // g_tw_lookahead
    // g_tw_nlp
    // g_tw_nkp
    // g_tw_synchronization_protocol

    // Set the two globals for the number of stations and stuff
    g_num_stations = get_station_count();
    // TODO: These should not map 1 to 1, but for now they do for simplicity
    g_num_transit_units = get_transit_unit_count();
    set_route_offset(g_num_stations);

    // Set the global time offset
    g_time_offset = get_g_start_time();

    //Given our total number of PEs figure out how many LPs should go to each
    total_nodes = tw_nnodes();

    // The idea here is to spread the stations evenly over the LPs
    num_lps_per_pe = (int)ceil((double)(g_num_stations + g_num_transit_units) / (double)total_nodes);


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

    graph_destroy();

    return 0;
}
