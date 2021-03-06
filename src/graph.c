// graph.c
// managing the graph

#include <igraph.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "graph.h"
#include "hash_table.h"
#include "graph_int.h"

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

void graph_init(const char* matrix_fn) {
    char line[256];
    char src_name[MAX_NAME_LEN]={0};
    char dst_name[MAX_NAME_LEN]={0};
    int delay;

    bool first_line = false;
    int dimension;
    long int i, j;
    int max_id=0;
    struct nlist* current;
    int src_id, dst_id;

    // Not totally clear if I need to do this, but here we are
    g_graph = (igraph_t *)calloc(1, sizeof(igraph_t));

    igraph_matrix_t mat;

    // Let's setup the name lookup, 0 it out
    memset(name_lookup_ar, 0, sizeof(name_lookup_ar[0][0]) * STA_MAX * MAX_NAME_LEN);

    // Crack open te intermediate file
    FILE* dat_file = fopen(matrix_fn, "r");
    if (dat_file == NULL) {
        perror("Failed to open matrix file!");
        exit(-1);
    }

    // Spin through and add all the edges descibed in the file
    while (fgets(line, sizeof(line), dat_file)) {
        // zero so we dont pick up garbage
        memset(src_name, 0, MAX_NAME_LEN);
        memset(dst_name, 0, MAX_NAME_LEN);
        delay = 0;

        // If it's the first line, it contains the dimension of the adj matrix
        if (first_line == false) {
            first_line = true;
            dimension = atoi(line);
            // Make a matrix object
            igraph_matrix_init(&mat, dimension, dimension);
            igraph_matrix_null(&mat);
            continue;
        }
        // For each line, get the values
        parse_dat_line(src_name, dst_name, &delay, line);

        // Did everything look ok space wise?
        if ((strlen(src_name) > MAX_NAME_LEN) || (strlen(dst_name) > MAX_NAME_LEN)) {
            perror("Attempted to load a station name that was too long!");
            exit(-1);
        }

        // Do we have an id for the src?
        current = lookup(id_lookup, src_name);
        if (current == NULL) {
            // No ID for src, take the current max
            src_id = max_id;
            max_id++; // Bump it up
            // Save it
            install(id_lookup, src_name, src_id);
        } else {
            // We have one, just use it
            src_id = current->defn;
        }
        // Repeat for the dst
        current = lookup(id_lookup, dst_name);
        if (current == NULL) {
            dst_id = max_id;

            max_id++; // Bump it up
            // Are we OK on the number of stations we loaded?
            if (max_id > STA_MAX) {
                perror("Attempted to load too many stations!");
                exit(-1);
            }

            install(id_lookup, dst_name, dst_id);
        } else {
            dst_id = current->defn;
        }

        // Let's save the reverse lookups
        strcpy(name_lookup_ar[src_id], src_name);
        strcpy(name_lookup_ar[dst_id], dst_name);
        name_lookup_ar_len = max_id;

        // Ok let's populate the matrix
        MATRIX(mat, src_id, dst_id) = delay;

    }
    fclose(dat_file);

    // Now, lets create the graph
    igraph_i_set_attribute_table(&igraph_cattribute_table);

    // Go ahead and generate the graph
    igraph_weighted_adjacency(g_graph, &mat, IGRAPH_ADJ_DIRECTED, 0, /*loops=*/ 1);
    // Print it so we can make sure it did what we thought
    //print(g_graph);

    g_station_count = dimension;

    // Dump the graph
    FILE* o_file = fopen("graph_dot_file", "w");
    igraph_write_graph_dot(g_graph, o_file);

}

void graph_destroy() {
    igraph_destroy(g_graph);
    free(g_graph);
}

/*
 * Lookup a name given an id
 */
char* sta_name_lookup(int id) {
    if(id > name_lookup_ar_len) {
        return NULL;
    }
    return name_lookup_ar[id];
}

/*
 * Lookup a name given an id and copy to dest
 */
int sta_name_copy(char* dest, int id) {
    if(id > name_lookup_ar_len) {
        return -1;
    }
    strcpy(dest, name_lookup_ar[id]);
    return 0;
}

/*
 * Lookup an id given a name
 */
int sta_id_lookup(char* dest) {
    return lookup(id_lookup, dest)->defn;
}

/*
 * Get the number of stations in the graph
 */
int get_station_count() {
    return g_station_count;
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

int parse_dat_line(char* src, char* dst, int* val, char* line) {
    char* curr; 

    // Get the src
    curr = strtok(line, " ");
    strcpy(src, curr);
    // Get the dest
    curr = strtok(NULL, " ");
    strcpy(dst, curr);
    // Get the weight and convert it
    curr = strtok(NULL, " ");
    *val = atoi(curr);

    return 0;
}
 
