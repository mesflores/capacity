// graph.c
// managing the graph

#include <igraph.h>
#include <string.h>
#include "graph.h"
#include "hash_table.h"
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
    struct nlist* new;
    char name;
    
    // Make a matrix object
    igraph_matrix_init(&mat, 10, 10);
    
    // Set everything in the matrix to the array
    for (i=0; i<10; i++) for (j=0; j<10; j++) MATRIX(mat, i, j) = m[i][j];
    
    igraph_i_set_attribute_table(&igraph_cattribute_table);
 
    // Go ahead and generate the graph
    igraph_weighted_adjacency(g_graph, &mat, IGRAPH_ADJ_DIRECTED, 0, /*loops=*/ 1);
    // Print it so we can make sure it did what we thought
    print(g_graph); 

    // Let's also go ahead and set up the ID lookup
    for (i=0; i<10; i++) {
        name = (char)i;
        new = install(id_lookup, &name, i);
    }

    // Let's setup the name lookup
    // 0 it out
    memset(name_lookup_ar, 0, sizeof(name_lookup_ar[0][0]) * 255 * 255);
    // Just put the i in for now, hard coded to 10
    for(i=0; i<10; i++) {
        // Ascii offset
        name_lookup_ar[i][0] = i + 48; 
    }
    name_lookup_ar_len = 10;

    // XXX XXX XXX XXX
}

void graph_destroy() {
    igraph_destroy(g_graph);
    free(g_graph);
}

/*
 * Lookup a name given an id
 */
int name_lookup(char* dest, int id) {
    if(id > name_lookup_ar_len) {
        return -1;
    }
    strcpy(dest, name_lookup_ar[id]);
    return 0;
}

/*
 * get the delay between two veritces 
 */
int get_delay_id(int src, int dest) {
    int edge_id;

    // Lookup the src and dest in the table
    igraph_get_eid(g_graph, &edge_id,
                   src, dest, IGRAPH_DIRECTED, 1);

    return (int)EAN(g_graph, "weight", edge_id);
}

int get_delay_name(char* src, char* dest) {
    int src_id, dest_id;   

    src_id = lookup(id_lookup, src)->defn;
    dest_id = lookup(id_lookup, dest)->defn;

    return get_delay_id(src_id, dest_id);
}
