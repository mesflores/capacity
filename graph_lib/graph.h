// graph.h
// definitions and management of the graph

#ifndef _graph_h
#define _graph_h


// TODO: this should take command line input or something
void graph_init();
void graph_destroy();


/* Meta info */
int name_lookup(char* dest, int id);

int get_station_count();

/* Graph Functiojns */
void find_path();
int get_delay_id(int src, int dest);
int get_delay_name(char* src, char* dest);

/* Dumb utility stuff */
int parse_dat_line(char* src, char* dst, int* val, char* line);

#endif
