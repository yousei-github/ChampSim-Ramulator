# Unit tests. One Catch2 executable collects every suite; each subdirectory's CMakeLists.txt
# attaches its own sources via target_sources(unit_tests ...), the same per-directory pattern
# source/ uses. A future Ramulator suite becomes a sibling subdirectory here.
add_executable(unit_tests)

# CHAMPSIM_TEST_BUILD marks a test binary. Upstream ChampSim uses it to elide main() and the
# generated-environment constants from src/main.cc; this fork keeps main.cc out of the test binary
# instead, so nothing reads the macro today. It is defined for parity with upstream test sources.
target_compile_definitions(unit_tests
    PRIVATE
    CHAMPSIM_TEST_BUILD)

# ${WarningErrorConfig} is deliberately omitted: the test sources are upstream's, and the "strict"
# preset must not turn their warnings into build failures.
target_compile_options(unit_tests
    PRIVATE
    ${WarningConfig}
    -fdiagnostics-color=always)

# champsim_core carries the include path and the third-party dependencies along with it.
# Catch2WithMain supplies main(), which is why source/main.cc is not part of the core library.
target_link_libraries(unit_tests
    PRIVATE
    champsim_core
    Catch2::Catch2WithMain)

set_target_properties(unit_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test/bin")

add_subdirectory(ChampSim)

# Catch.cmake ships with Catch2 and lands on CMAKE_MODULE_PATH via find_package(Catch2).
# catch_discover_tests() registers every TEST_CASE / SCENARIO with ctest individually.
include(Catch)
catch_discover_tests(unit_tests)
