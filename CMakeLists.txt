# The cmake_minimum_required(VERSION 3.xx) is a compatibility guarantee provided by CMake to the project developer.
# When called, it ensures that CMake will adopt the behavior of the listed version.
# If a later version of CMake is invoked on a CML containing the above code, it will act exactly as if it were CMake 3.xx.
cmake_minimum_required(VERSION 3.20)

# The project() command is a conceptually simple command which provides a complex function.
# It informs CMake that what follows is the description of a distinct software project of a given name (as opposed to a shell-like script).
# When CMake sees the project() command it performs various checks to ensure the environment is suitable for building software;
# such as checking for compilers and other build tooling, and discovering properties like the endianness of the host and target machines.
project(champsim_plus_ramulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CCACHE_PROJECT_DIR "${CMAKE_SOURCE_DIR}/.ccache")
    set(CMAKE_C_COMPILER_LAUNCHER
        "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_PROJECT_DIR}" "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_PROJECT_DIR}" "${CCACHE_PROGRAM}")
    message(STATUS "ccache enabled: ${CCACHE_PROGRAM} (cache dir: ${CCACHE_PROJECT_DIR})")
endif()

find_package(fmt      CONFIG REQUIRED)
find_package(OpenMP          REQUIRED)
find_package(BZip2           REQUIRED)
find_package(LibLZMA         REQUIRED)
find_package(ZLIB            REQUIRED)

# Building an executable
add_executable(champsim_plus_ramulator) # This command creates a target. In CMake lingo, a target is a name the developer gives to a collection of properties

target_include_directories(champsim_plus_ramulator
    PRIVATE
    "${CMAKE_SOURCE_DIR}/include")

target_compile_options(champsim_plus_ramulator
    PRIVATE
    -Wall -fdiagnostics-color=always)

# Combine libraries with the executable
target_link_libraries(champsim_plus_ramulator
    PRIVATE
    fmt::fmt
    OpenMP::OpenMP_CXX
    LibLZMA::LibLZMA
    BZip2::BZip2
    ZLIB::ZLIB)

set_target_properties(champsim_plus_ramulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Pull in per-directory source lists. Each subdirectory's CMakeLists.txt
# attaches its files to the executable via target_sources().
add_subdirectory(source)
