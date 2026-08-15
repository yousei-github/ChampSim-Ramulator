# main.cc owns main(), so it belongs to the simulator executable alone.
# The ChampSim unit test executable gets its own main() from Catch2::Catch2WithMain and must not link this file.
target_sources(${EXECUTABLE_NAME}
    PRIVATE
    main.cc)

target_sources(${CHAMPSIM_TARGET}
    PRIVATE
    simulator_statistics.cc)

add_subdirectory(ChampSim)
add_subdirectory(OS_Transparent_Management)
add_subdirectory(Ramulator)
add_subdirectory(Ramulator2)
