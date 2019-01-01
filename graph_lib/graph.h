// graph.h
// definitions and management of the graph

#ifndef _graph_h
#define _graph_h

typedef struct graph_node_t {
    struct graph_node_t* next;
    int vert_id;
    int weight;
} graph_node_t;

typedef struct adj_list_t {
    graph_node_t* list;
} adj_list_t;

typedef struct graph_t {
    adj_list_t* vertices;
    int vert_count;
} graph_t;

graph_t* g_graph_t;

// TODO: this should take command line input or something
void graph_init();

void add_edge(graph_t* graph, int src, int dest, int weight);

int lookup_weight(graph_t* graph, int src, int dest);

#endif
