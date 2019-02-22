# This file is part of VROOM.
#
# Copyright (c) 2015-2018, Julien Coupey.
# All rights reserved (see LICENSE).

cmake_minimum_required(VERSION 2.8.12) # first version with add_compile_options()
project(vroom)



# Global settings
set(GLOBAL_OUTPUT_PATH ${PROJECT_BINARY_DIR}/output)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install)

# Sets global output directory for single configuration (GCC)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})

# Sets global output directory for sub-configurations (msvc, mingw)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
  message(STATUS "Building on a 64 bit system")
else()
  message(STATUS "Building on a 32 bit system")
endif()

# 3rdparty Libraries
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MMD -MP -Wextra -Wpedantic -Wall -O3 -DBOOST_SPIRIT_USE_PHOENIX_V3 -DBOOST_RESULT_OF_USE_DECLTYPE -DBOOST_FILESYSTEM_NO_DEPRECATED -fuse-ld=gold -Wl,--disable-new-dtags  -Wl,--gc-sections -Wl,-O1 -Wl,--hash-style=gnu -Wl,--sort-common")

include(CMakeLists-ExternalProjects.txt)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message (STATUS "CMake version: ${CMAKE_VERSION}")


# Print CXX compiler information
message (STATUS "CMAKE_CXX_COMPILER '${CMAKE_CXX_COMPILER}' '${CMAKE_CXX_COMPILER_ID}' '${CMAKE_CXX_COMPILER_VERSION}'")

# Print CXX FLAGS
message (STATUS "CMAKE_CXX_FLAGS                '${CMAKE_CXX_FLAGS}'")
if (MSVC)
    message (STATUS "CMAKE_CXX_FLAGS_DEBUG          '${CMAKE_CXX_FLAGS_DEBUG}'")
    message (STATUS "CMAKE_CXX_FLAGS_RELEASE        '${CMAKE_CXX_FLAGS_RELEASE}'")
    message (STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO '${CMAKE_CXX_FLAGS_RELWITHDEBINFO}'")
    message (STATUS "CMAKE_CXX_FLAGS_MINSIZEREL     '${CMAKE_CXX_FLAGS_MINSIZEREL}'")
else (NOT MSVC)
    if     (CMAKE_BUILD_TYPE STREQUAL Debug)
        message (STATUS "CMAKE_CXX_FLAGS_DEBUG          '${CMAKE_CXX_FLAGS_DEBUG}'")
    elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
        message (STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO '${CMAKE_CXX_FLAGS_RELWITHDEBINFO}'")
    elseif (CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
        message (STATUS "CMAKE_CXX_FLAGS_MINSIZEREL     '${CMAKE_CXX_FLAGS_MINSIZEREL}'")
    else ()
        message (STATUS "CMAKE_CXX_FLAGS_RELEASE        '${CMAKE_CXX_FLAGS_RELEASE}'")
    endif ()
endif ()

## Build the C++ Wrapper ##

# adding a new file require explicittly modifing the CMakeLists.txt
# so that CMake knows that it should rebuild the project (it is best practice)

# list of sources files of the library
set(VROOM_SRC
        # algorithms
        ${PROJECT_SOURCE_DIR}/src/algorithms/kruskal.cpp
        ${PROJECT_SOURCE_DIR}/src/algorithms/munkres.cpp
        ${PROJECT_SOURCE_DIR}/src/algorithms/heuristics/clustering.cpp
        ${PROJECT_SOURCE_DIR}/src/algorithms/heuristics/solomon.cpp
        ${PROJECT_SOURCE_DIR}/src/algorithms/local_search/local_search.cpp
        ${PROJECT_SOURCE_DIR}/src/algorithms/local_search/operator.cpp
        # problems
        ${PROJECT_SOURCE_DIR}/src/problems/vrp.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/cvrp.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/cross_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_cross_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_mixed_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_or_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_relocate.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/mixed_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/or_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/relocate.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/reverse_two_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/two_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/tsp.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/heuristics/local_search.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/heuristics/christofides.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/vrptw.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/two_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/reverse_two_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/relocate.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/or_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/mixed_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_relocate.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_or_opt.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_mixed_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_cross_exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/exchange.cpp
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/cross_exchange.cpp
        # routing
        ${PROJECT_SOURCE_DIR}/src/routing/orshttp_wrapper.cpp
        ${PROJECT_SOURCE_DIR}/src/routing/routed_wrapper.cpp
         ${PROJECT_SOURCE_DIR}/src/routing/libosrm_wrapper.cpp
        # structure
        ${PROJECT_SOURCE_DIR}/src/structures/cl_args.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/generic/edge.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/generic/matrix.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/generic/undirected_graph.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/job.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/location.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution_state.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/time_window.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/tw_route.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/vehicle.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/computing_times.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/route.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/solution.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/step.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/summary.cpp
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/input/input.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/exception.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/input_parser.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/output_json.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/version.cpp
)
source_group(src FILES ${VROOM_SRC})


# list of header files of the library
set(VROOM_INC
        # algorithms
        ${PROJECT_SOURCE_DIR}/src/algorithms/kruskal.h
        ${PROJECT_SOURCE_DIR}/src/algorithms/munkres.h
        ${PROJECT_SOURCE_DIR}/src/algorithms/heuristics/clustering.h
        ${PROJECT_SOURCE_DIR}/src/algorithms/heuristics/solomon.h
        ${PROJECT_SOURCE_DIR}/src/algorithms/local_search/local_search.h
        ${PROJECT_SOURCE_DIR}/src/algorithms/local_search/operator.h
        # problems
        ${PROJECT_SOURCE_DIR}/src/problems/vrp.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/cvrp.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/cross_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_cross_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_mixed_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_or_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/intra_relocate.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/mixed_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/or_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/relocate.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/reverse_two_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/cvrp/operators/two_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/tsp.h
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/heuristics/local_search.h
        ${PROJECT_SOURCE_DIR}/src/problems/tsp/heuristics/christofides.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/vrptw.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/two_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/reverse_two_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/relocate.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/or_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/mixed_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_relocate.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_or_opt.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_mixed_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/intra_cross_exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/exchange.h
        ${PROJECT_SOURCE_DIR}/src/problems/vrptw/operators/cross_exchange.h
        # routing
        ${PROJECT_SOURCE_DIR}/src/routing/wrapper.h
        ${PROJECT_SOURCE_DIR}/src/routing/orshttp_wrapper.h
        ${PROJECT_SOURCE_DIR}/src/routing/routed_wrapper.h
        ${PROJECT_SOURCE_DIR}/src/routing/libosrm_wrapper.h
        ${PROJECT_SOURCE_DIR}/src/routing/osrm_wrapper.h
        # structure
        ${PROJECT_SOURCE_DIR}/src/structures/typedefs.h
        ${PROJECT_SOURCE_DIR}/src/structures/cl_args.h
        ${PROJECT_SOURCE_DIR}/src/structures/generic/edge.h
        ${PROJECT_SOURCE_DIR}/src/structures/generic/matrix.h
        ${PROJECT_SOURCE_DIR}/src/structures/generic/undirected_graph.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/job.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/location.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution_state.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/time_window.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/tw_route.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/amount.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/vehicle.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/raw_route.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/computing_times.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/route.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/solution.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/step.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/solution/summary.h
        ${PROJECT_SOURCE_DIR}/src/structures/vroom/input/input.h
        ${PROJECT_SOURCE_DIR}/src/utils/exception.h
        ${PROJECT_SOURCE_DIR}/src/utils/input_parser.h
        ${PROJECT_SOURCE_DIR}/src/utils/output_json.h
        ${PROJECT_SOURCE_DIR}/src/utils/version.h
        ${PROJECT_SOURCE_DIR}/src/utils/helpers.h
)

source_group(include FILES ${VROOM_INC})
# list of test files of the library

# list of example files of the library
set(VROOM_EXAMPLES
        ${PROJECT_SOURCE_DIR}/libvroom_examples/libvroom.cpp)

source_group(example1 FILES ${VROOM_EXAMPLES})
# list of script files of the library

find_package(Boost 1.54.0 COMPONENTS system filesystem iostreams thread REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB REQUIRED)

# All includes are relative to the "include" directory
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${Boost_INCLUDE_DIR})

# Global link directories
link_directories(AFTER ${GLOBAL_OUTPUT_PATH})

# Add targets
# 
# Add the vroom library
#
add_library(vroom ${VROOM_SRC} ${VROOM_INC})
target_link_libraries(vroom ${Boost_LIBRARIES} ${OSRM_LIBRARIES})
add_dependencies(vroom RapidJSON OSRM)

# 
# Add the vroom server application
#
add_executable(vroom-server src/main.cpp)
target_link_libraries(vroom-server vroom ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} ${TBB_LIBRARIES})
if(UNIX AND NOT APPLE)
    target_link_libraries(vroom-server rt)
endif()

option(VROOM_BUILD_EXAMPLES "Build examples." OFF)
if (VROOM_BUILD_EXAMPLES)
    # add the basic example executable
    add_executable(vroom_example1 ${VROOM_EXAMPLES})
    target_link_libraries(vroom_example1 vroom)
    # Link target with pthread and dl for linux
    if (UNIX)
        target_link_libraries(vroom_example1 pthread)
    elseif (MSYS OR MINGW)
        target_link_libraries(vroom_example1 ssp)
    endif ()
else (VROOM_BUILD_EXAMPLES)
    message(STATUS "VROOM_BUILD_EXAMPLES OFF")
endif (VROOM_BUILD_EXAMPLES)
