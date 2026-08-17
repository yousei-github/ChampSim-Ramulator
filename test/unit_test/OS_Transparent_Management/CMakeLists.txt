# OS-transparent management unit tests. The leading number groups a test by research proposal;
# see README.md. Keep this list sorted by file name.
target_sources(${UNIT_TEST_EXECUTABLE_NAME}
    PRIVATE
    100-base-remapping-queue.cc
    200-cameo-translation.cc
    201-cameo-hotness-and-enqueue.cc
    202-cameo-llt-permutation.cc
    210-colocated-incomplete-queues.cc
    300-vg-granularity-ladders.cc
    301-vg-translation.cc
    400-mempod-mea-counter.cc
    401-mempod-interval-swap.cc
    500-static-placement-baseline.cc)

# test_capacities.hpp lives beside the sources that include it.
target_include_directories(${UNIT_TEST_EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})
