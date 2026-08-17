# Unit tests. One Catch2 executable collects every suite; each subdirectory's CMakeLists.txt
# attaches its own sources via target_sources(${UNIT_TEST_EXECUTABLE_NAME} ...), the same per-directory pattern
# source/ uses. A future Ramulator suite becomes a sibling subdirectory here.

# Customizable executable name (CMake Cache, string variable, override with -D UNIT_TEST_EXECUTABLE_NAME=<name> at configure time)
set(UNIT_TEST_EXECUTABLE_NAME "champsim_unit_tests" CACHE STRING "Name of the output executable of unit test")

message(STATUS "UNIT_TEST_EXECUTABLE_NAME: ${UNIT_TEST_EXECUTABLE_NAME}")

add_executable(${UNIT_TEST_EXECUTABLE_NAME})

# CHAMPSIM_TEST_BUILD marks a test binary. Upstream ChampSim uses it to elide main() and the
# generated-environment constants from src/main.cc; this fork keeps main.cc out of the test binary
# instead, so nothing reads the macro today. It is defined for parity with upstream test sources.
target_compile_definitions(${UNIT_TEST_EXECUTABLE_NAME}
    PRIVATE
    CHAMPSIM_TEST_BUILD)

# ${WarningErrorConfig} is deliberately omitted: the test sources are upstream's, and the "strict"
# preset must not turn their warnings into build failures.
target_compile_options(${UNIT_TEST_EXECUTABLE_NAME}
    PRIVATE
    ${WarningConfig}
    -fdiagnostics-color=always)

# champsim_core carries the include path and the third-party dependencies along with it.
# Catch2WithMain supplies main(), which is why source/main.cc is not part of the core library.
target_link_libraries(${UNIT_TEST_EXECUTABLE_NAME}
    PRIVATE
    champsim_core
    Catch2::Catch2WithMain)

set_target_properties(${UNIT_TEST_EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test/bin")

add_subdirectory(ChampSim)

# Catch.cmake ships with Catch2 and lands on CMAKE_MODULE_PATH via find_package(Catch2).
# catch_discover_tests() registers every TEST_CASE / SCENARIO with ctest individually.
include(Catch)
catch_discover_tests(${UNIT_TEST_EXECUTABLE_NAME})
