// graph.c
// managing the graph

#include "ross.h"
#include "graph.h"

void graph_init() {
    int i;

    // XXX XXX
    // As a starter let's just encode a big list of 10 vertices connected
    // in numerical order, ie 1 to 2 to 3 to 4... to 10
    // XXX XXX
    
    // Alloate the main graph struture
    g_graph_t = tw_calloc(TW_LOC, "", sizeof(graph_t), 1);
    // Set the vert count TODO: make this not as stupid
    g_graph_t->vert_count = 10;
    g_graph_t->vertices = tw_calloc(TW_LOC, "", sizeof(adj_list_t), g_graph_t->vert_count);
    // Loop through and null out the lists
    for (i=0; i < g_graph_t->vert_count; i++) {
        g_graph_t->vertices[i].list = NULL;
    }

    // TODO: This is just shitty hardcoding
    for (i=0; i < (g_graph_t->vert_count - 1); i++) {
        add_edge(g_graph_t, i, i+1, 10); 
    }
    for (i=1; i < (g_graph_t->vert_count ); i++) {
        add_edge(g_graph_t, i, i-1, 10); 
    }

    
}

/*
 * add an edge to the graph 
 */
void add_edge(graph_t* graph, int src, int dest, int weight) {
    graph_node_t* new_edge; 

    // Carve us a home for it
    new_edge = tw_calloc(TW_LOC, "", sizeof(graph_node_t), 1);
    new_edge->vert_id = dest;
    // Save the weight
    new_edge->weight = weight;
    // stuff it into the list
    new_edge->next = graph->vertices[src].list;
    graph->vertices[src].list = new_edge;
}

/* 
 *  lookup_weight - given a src and dest, lookup the weight of the link between
 *  them. Returns -1 if no link is found.
 */
int lookup_weight(graph_t* graph, int src, int dest) {
    graph_node_t* curr_vertex; 

    curr_vertex = (graph->vertices)[src].list;

    while (curr_vertex != NULL) {
        if (curr_vertex->vert_id == dest) {
            return curr_vertex->weight;
        }
    }
    // We never found a match, bail
    return -1;
}


