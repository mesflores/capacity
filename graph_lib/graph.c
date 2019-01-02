// graph.c
// managing the graph

#include <igraph.h>
#include "graph.h"
#include "graph_internal.h"

/* Nabbed from the igraph examples, very handy */
void print(igraph_t *g) {
  igraph_vector_t el;
  long int i, j, n;
  char ch = igraph_is_directed(g) ? '>' : '-';

  igraph_vector_init(&el, 0);
  igraph_get_edgelist(g, &el, 0);
  n = igraph_ecount(g);

  for (i=0, j=0; i<n; i++, j+=2) {
    printf("%ld --%c %ld: %ld\n",
      (long)VECTOR(el)[j], ch, (long)VECTOR(el)[j+1], (long)EAN(g, "weight", i)); 
  }
  printf("\n");

  igraph_vector_destroy(&el);
}

void graph_init() {
    // Make a basic empty graph
    // Not totally clear if I need to do this, but here we are
    g_graph = (igraph_t *)calloc(1, sizeof(igraph_t));


    // XXX XXX XXX XXX
    // For now, let's just add some hard coded stuff
    igraph_matrix_t mat;

    int m[10][10] = { { 0, 10, 12, 0, 0, 0, 0, 0, 0, 0}, 
                      { 10, 0, 10, 0, 0, 0, 0, 0, 0, 0},
                      { 0, 10, 0, 10, 0, 0, 0, 0, 0, 0},
                      { 0, 0, 10, 0, 10, 0, 0, 0, 0, 0},
                      { 0, 0, 0, 10, 0, 10, 0, 0, 0, 0},
                      { 0, 0, 0, 0, 10, 0, 10, 0, 0, 0},
                      { 0, 0, 0, 0, 0, 10, 0, 10, 0, 0},
                      { 0, 0, 0, 0, 0, 0, 10, 0, 10, 0},
                      { 0, 0, 0, 0, 0, 0, 0, 10, 0, 10},
                      { 0, 0, 0, 0, 0, 0, 0, 0, 10, 0},
                    };
    long int i, j;
    
    // Make a matrix object
    igraph_matrix_init(&mat, 10, 10);
    
    // Set everything in the matrix to the array
    for (i=0; i<10; i++) for (j=0; j<10; j++) MATRIX(mat, i, j) = m[i][j];
    
    igraph_i_set_attribute_table(&igraph_cattribute_table);
 
    // Go ahead and generate the graph
    igraph_weighted_adjacency(g_graph, &mat, IGRAPH_ADJ_DIRECTED, 0, /*loops=*/ 1);
    // Print it so we can make sure it did what we thought
    print(g_graph); 

    // XXX XXX XXX XXX
}

/*
 * get the delat between two veritces 
 */
int get_delay(int src, int dest) {
    // TODO: Right now this assumes that the src and dest are IDs 
    int edge_id;

    igraph_get_eid(g_graph, &edge_id,
                   src, dest, IGRAPH_DIRECTED, 1);

    return (int)EAN(g_graph, "weight", edge_id);
}
