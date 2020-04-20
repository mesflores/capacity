INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})
## Must include binary dir for config.h

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/Modules/")

## If you will be using the RIO library add this
#INCLUDE_DIRECTORIES(${RIO_SOURCE_DIR} ${RIO_BINARY_DIR})

### Model Version
#LIST(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
#INCLUDE(GetGitRevisionDescription)
#GET_GIT_HEAD_REVISION(GIT_RESPEC_MODEL GIT_SHA1_MODEL)
#
### Build Configuration Header File
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
#CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/model_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/model_config.h @ONLY)
#SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/model_config.h.in PROPERTIES GENERATED FALSE)
#SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/model_config.h PROPERTIES GENERATED TRUE)


# All source files for the model
SET(capacity_srcs
    cap_definitions.h
    capacity_main.c
    passenger.h
    passenger.c
    transit_unit.c
    station.c
    track.c
    track.h
    utils.h
    utils.c
    model.h
    model_mapping.c
	model_config.h
)

# Internal lib stuff
SET(capacity_libs
    graph.h
    graph_int.h
    graph.c
    hash_table.c
    hash_table.h
    route.h
    route.c
    route_int.h
)

# For the tests
SET(test_src
    test.c
)

# Add the libraries
find_package(igraph REQUIRED)

include_directories(${IGRAPH_INCLUDE_DIR})
set(LIBS ${IGRAPH_LIBRARIES})

#TARGET_LINK_LIBRARIES(ROSS ${LIBS})

# Set the executable name here: model
# You will be executing the ./model binary
ADD_EXECUTABLE(capacity ${capacity_srcs} ${capacity_libs})
TARGET_LINK_LIBRARIES(capacity ROSS m ${LIBS})

ADD_EXECUTABLE(test_exe ${test_src} ${capacity_libs})
TARGET_LINK_LIBRARIES(test_exe ROSS m ${LIBS})
## Alternatively, use this command with RIO
#TARGET_LINK_LIBRARIES(model ROSS RIO m)

# Add some tests
add_definitions(-DTEST_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test_data/)
ADD_TEST(NAME capacity_test COMMAND test_exe)
