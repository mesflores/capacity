// graph_internal.h -- stuff for the graph, internal only

#ifndef _graph_internal_h
#define _graph_internal_h

igraph_t* g_graph;
static struct nlist *id_lookup[HASHSIZE];
char name_lookup_ar[255][255]; // TODO: These are stupid sizes
int name_lookup_ar_len=0;

void print(igraph_t *);

#endif
