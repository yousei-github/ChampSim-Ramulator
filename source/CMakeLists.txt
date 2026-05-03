target_sources(champsim_plus_ramulator
    PRIVATE
    main.cc
    ProjectConfiguration.cc
    cameo.cc
    os_transparent_management.cc
    variable_granularity.cc
    ideal_single_mempod.cc)

add_subdirectory(ChampSim)
add_subdirectory(Ramulator)