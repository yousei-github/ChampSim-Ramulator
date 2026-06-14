target_sources(${EXECUTABLE_NAME}
    PRIVATE
    main.cc
    ProjectConfiguration.cc
    cameo.cc
    os_transparent_management.cc
    variable_granularity.cc
    ideal_single_mempod.cc)

add_subdirectory(ChampSim)
add_subdirectory(Ramulator)
add_subdirectory(Ramulator2)