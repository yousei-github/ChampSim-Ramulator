# main.cc owns main(), so it belongs to the simulator executable alone. The unit test executable
# gets its own main() from Catch2::Catch2WithMain and must not link this file.
target_sources(${EXECUTABLE_NAME}
    PRIVATE
    main.cc)

# simulator_statistics.cc defines the output_statistics / output_memorytrace globals that the
# PRINTF_STATISTICS_FILE call sites all over the simulator reference, so it belongs to the core.
target_sources(${CHAMPSIM_TARGET}
    PRIVATE
    simulator_statistics.cc)

add_subdirectory(ChampSim)
add_subdirectory(OS_Transparent_Management)
add_subdirectory(Ramulator)
add_subdirectory(Ramulator2)
