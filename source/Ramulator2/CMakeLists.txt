# Vendored Ramulator 2.0, in-tree under source/Ramulator2/ (sources) and
# include/Ramulator2/ (headers). Mirrors the project's existing split for v1
# (source/Ramulator/ + include/Ramulator/).
#
# The only .cpp directly in this directory is main.cpp; it is wrapped in
#   #if (USER_CODES != ENABLE) ... #endif
# so it compiles to an empty translation unit.

target_sources(${EXECUTABLE_NAME}
    PRIVATE
    main.cpp)

add_subdirectory(addr_mapper)
add_subdirectory(base)
add_subdirectory(dram)
add_subdirectory(dram_controller)
add_subdirectory(example)
add_subdirectory(frontend)
add_subdirectory(memory_system)
add_subdirectory(test)
add_subdirectory(translation)
