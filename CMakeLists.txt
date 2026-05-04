# The cmake_minimum_required(VERSION 3.xx) is a compatibility guarantee provided by CMake to the project developer.
# When called, it ensures that CMake will adopt the behavior of the listed version.
# If a later version of CMake is invoked on a CML containing the above code, it will act exactly as if it were CMake 3.xx.
cmake_minimum_required(VERSION 3.21)

# The project() command is a conceptually simple command which provides a complex function.
# It informs CMake that what follows is the description of a distinct software project of a given name (as opposed to a shell-like script).
# When CMake sees the project() command it performs various checks to ensure the environment is suitable for building software;
# such as checking for compilers and other build tooling, and discovering properties like the endianness of the host and target machines.
# 
# Options:
# - LANGUAGES <language-name>...
# Selects which programming languages are needed to build the project.
# Supported languages are C, CXX (i.e. C++), CSharp (i.e. C#), CUDA, OBJC (i.e. Objective-C), OBJCXX (i.e. Objective-C++),
# Fortran, HIP, ISPC, Swift, ASM, ASM_NASM, ASM_MARMASM, ASM_MASM, and ASM-ATT.
# 
# - VERSION <version>
# Takes a <version> argument composed of non-negative integer components, i.e. <major>[.<minor>[.<patch>[.<tweak>]]],
# and sets the variables PROJECT_VERSION, PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH, PROJECT_VERSION_TWEAK.
project(champsim_plus_ramulator LANGUAGES CXX)

# Build system options
# Option provides a boolean option that the user can optionally select.
option(ENABLE_ASSERT "Enable assertion" ON) # Assertion Configuration
option(SHOW_All_WARNING "Show all optional warnings which are desirable for normal code" ON) # Warnings Configuration
option(ENABLE_All_WARNING_ERROR "Make all warnings into hard errors" OFF) # Warning error Configuration

# Output build system options
message(STATUS "ENABLE_ASSERT: ${ENABLE_ASSERT}")
message(STATUS "SHOW_All_WARNING: ${SHOW_All_WARNING}")
message(STATUS "ENABLE_All_WARNING_ERROR: ${ENABLE_All_WARNING_ERROR}")

# Convert build system options into flags for compiler, CMake, and so on
if(ENABLE_ASSERT)
    set(AssertConfig "")
else()
    set(AssertConfig "-DNDEBUG")
endif()

if(SHOW_All_WARNING)
    set(WarningConfig "-Wall")
else()
    set(WarningConfig "")
endif()

if(ENABLE_All_WARNING_ERROR)
    set(WarningErrorConfig "-Werror")
else()
    set(WarningErrorConfig "")
endif()

# C++ language standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g ${AssertConfig}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g ${AssertConfig}")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Create a variable, which is to say a name for a string
    set(CCACHE_PROJECT_DIR "${CMAKE_SOURCE_DIR}/.ccache")
    set(CMAKE_C_COMPILER_LAUNCHER
        "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_PROJECT_DIR}" "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_PROJECT_DIR}" "${CCACHE_PROGRAM}")
    
    # A variable's value can be accessed using brace expansion. Use the message() command to print the string named by CCACHE_PROJECT_DIR
    message(STATUS "ccache enabled: ${CCACHE_PROGRAM} (cache dir: ${CCACHE_PROJECT_DIR})")
else()
    message(STATUS "ccache is not installed (No speed up for recompilation)")
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
    ${WarningConfig}
    ${WarningErrorConfig}
    -fdiagnostics-color=always)

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
