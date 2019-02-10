#include <igraph.h> 
#include <stdio.h>
#include "graph_lib/graph.h"
#include "graph_lib/hash_table.h"
#include "graph_lib/route.h"

int main(int argc, char* argv[]) { 
    //graph_init();
    //graph_destroy();

    init_global_routes();
    print_global_routes();

    return 0;
}
