// graph_internal.h -- stuff for the graph, internal only

#ifndef _graph_internal_h
#define _graph_internal_h

// Maximum number of stations
#define STA_MAX 512
#define MAX_NAME_LEN 128

// The graph object itself
igraph_t* g_graph;
// The Forward name lookup that returns an ID given a station name
static struct nlist *id_lookup[HASHSIZE];
// Array for looking up station names given an ID
char name_lookup_ar[STA_MAX][MAX_NAME_LEN]; // TODO: These are stupid sizes
int name_lookup_ar_len=0;

// Toty
int g_station_count=0;

void print(igraph_t *);

#endif
