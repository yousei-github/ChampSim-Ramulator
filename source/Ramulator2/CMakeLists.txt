# Vendored Ramulator 2.0, in-tree under source/Ramulator2/ (sources) and
# include/Ramulator2/ (headers). Mirrors the project's existing split for v1
# (source/Ramulator/ + include/Ramulator/).
#
# Files we don't compile (upstream main.cpp, test/, example/, and
# dram_controller/impl/plugin/trr.cpp) are kept on disk and wrapped in
#   #if (USER_CODES != ENABLE) ... #endif

file(GLOB_RECURSE R2_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

target_sources(champsim_plus_ramulator
    PRIVATE
    ${R2_SOURCES})

# target_sources(champsim_plus_ramulator
#     PRIVATE
#     main.cpp)

# add_subdirectory(addr_mapper/impl)
# add_subdirectory(base)
# add_subdirectory(dram/impl)
# add_subdirectory(dram_controller/impl)
# add_subdirectory(dram_controller/impl/plugin)
# add_subdirectory(dram_controller/impl/plugin/bliss)
# add_subdirectory(dram_controller/impl/plugin/blockhammer)
# add_subdirectory(dram_controller/impl/plugin/device_config)
# add_subdirectory(dram_controller/impl/plugin/prac)
# add_subdirectory(dram_controller/impl/refresh)
# add_subdirectory(dram_controller/impl/rowpolicy)
# add_subdirectory(dram_controller/impl/scheduler)
# add_subdirectory(example/impl)
# add_subdirectory(frontend/impl/external_wrapper)
# add_subdirectory(frontend/impl/memory_trace)
# add_subdirectory(frontend/impl/processor/bhO3)
# add_subdirectory(frontend/impl/processor/simpleO3)
# add_subdirectory(memory_system)
# add_subdirectory(test)
# add_subdirectory(translation/impl)