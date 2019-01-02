// graph_internal.h -- stuff for the graph, internal only

#ifndef _graph_internal_h
#define _graph_internal_h

igraph_t* g_graph;
static struct nlist *id_lookup[HASHSIZE];

void print(igraph_t *);

#endif
