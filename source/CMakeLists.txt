target_sources(${EXECUTABLE_NAME}
    PRIVATE
    main.cc
    simulator_statistics.cc)

add_subdirectory(ChampSim)
add_subdirectory(OS_Transparent_Management)
add_subdirectory(Ramulator)
add_subdirectory(Ramulator2)