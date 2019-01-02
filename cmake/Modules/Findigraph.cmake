# Try to find igraph
# Include directory
find_path(IGRAPH_INCLUDE_DIR NAMES igraph.h
          PATHS /usr/local/include/igraph)

# Finally the library itself
find_library(IGRAPH_LIBRARIES NAMES igraph libigraph)
