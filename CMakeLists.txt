cmake_minimum_required(VERSION 3.20)
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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/source/*.cc"
     "${CMAKE_SOURCE_DIR}/source/*.cpp")

add_executable(champsim_plus_ramulator ${SOURCES})

target_include_directories(champsim_plus_ramulator PRIVATE
    "${CMAKE_SOURCE_DIR}/include")

target_compile_options(champsim_plus_ramulator PRIVATE
    -Wall -fdiagnostics-color=always)

target_link_libraries(champsim_plus_ramulator PRIVATE
    fmt::fmt
    OpenMP::OpenMP_CXX
    LibLZMA::LibLZMA
    BZip2::BZip2
    ZLIB::ZLIB)

set_target_properties(champsim_plus_ramulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
