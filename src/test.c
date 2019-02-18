#include <igraph.h> 
#include <stdio.h>
#include "graph.h"
#include "hash_table.h"
#include "route.h"

// Convert our arg from cmake to a usable string
#define STR_VALUE(arg)      #arg
#define STR2(arg)    STR_VALUE(arg)
#define PATH STR2(TEST_DATA_PATH)

int graph_test();

int graph_test() {
    int delay;

    printf("Test Data: %s\n", PATH);

    // Init the simple 2x2
    printf("2x2 test...");
    graph_init(PATH "2_by_2.mat");
    // Look up a delay
    delay = get_delay_name("1", "2");
    if (delay != 10) {
        return -1;
    }
    // Check the other way
    delay = get_delay_name("2", "1");
    if (delay != 5) {
        return -1;
    }
    graph_destroy();

    return 0;

}

int main(int argc, char* argv[]) { 
    int ret_val;

    ret_val = graph_test();
    if (ret_val != 0) {
        return ret_val;
    }

    return 0;
}
