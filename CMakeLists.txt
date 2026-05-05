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
option(ENABLE_All_WARNING_ERROR "Make all warnings into hard errors" OFF) # Warnings-as-error Configuration

# Output build system options
message(STATUS "ENABLE_ASSERT: ${ENABLE_ASSERT}")
message(STATUS "SHOW_All_WARNING: ${SHOW_All_WARNING}")
message(STATUS "ENABLE_All_WARNING_ERROR: ${ENABLE_All_WARNING_ERROR}")

# Convert build system options into flags for compiler, CMake, and so on
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
# See https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#manual:cmake-properties(7) for more details
set(CMAKE_CXX_STANDARD 17) # This property specifies the C++ standard whose features are requested to build this target. Supported values are: 98 (C++98), 11 (C++11), 14 (C++14), 17 (C++17), 20 (C++20), 23 (C++23), 26 (C++26)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # If this property is set to ON, then the value of the CXX_STANDARD target property is treated as a requirement. If this property is OFF or unset, the CXX_STANDARD target property is treated as optional and may "decay" to a previous standard if the requested is not available
set(CMAKE_CXX_EXTENSIONS OFF) # This property specifies whether compiler specific extensions should be used. For some compilers, this results in adding a flag such as -std=gnu++11 instead of -std=c++11 to the compile line
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # If enabled, generates a compile_commands.json file containing the exact compiler calls for all translation units of the project in machine-readable form

if(NOT CMAKE_BUILD_TYPE)
    # Specifies the build type on single-configuration generators (e.g. Makefile Generators or Ninja). Typical values include Debug, Release, RelWithDebInfo and MinSizeRel, but custom build types can also be defined
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# CMAKE_<LANG>_FLAGS_<CONFIG>
# Language-wide flags for language <LANG> used when building for the <CONFIG> configuration.
# These flags will be passed to all invocations of the compiler in the corresponding configuration.
# This includes invocations that drive compiling and those that drive linking.
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g") # [-O0] achieves no optimization, maximum size & slowest speed
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os") # [-Os] achieves balance of speed and small size

# Check Source Compile (C++)
include(CheckSourceCompiles) # Load the module that checks whether a source code can be built for a given language in a CMake project
# Checks once whether the given source code can be built for the given language
check_source_compiles(CXX
  "
    int main() {
      int a, b, c;
      __builtin_add_overflow(a, b, &c);
    }
  "
  HAS_CHECKED_ADDITION
)

if(HAS_CHECKED_ADDITION)
    message(STATUS "Source compile check: Pass")
else()
    message(STATUS "Source compile check: Fail")
endif()

# Compiler identification
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT)
    message(STATUS "C++ compiler frontend variant: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")
endif()

# Find ccache
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

# Finds and reports the full path to a package (file / static archive / shared object, usually provided by something external to the project) suitable for use with target_link_libraries()
find_package(fmt      CONFIG REQUIRED)
find_package(OpenMP          REQUIRED)
find_package(BZip2           REQUIRED)
find_package(LibLZMA         REQUIRED)
find_package(ZLIB            REQUIRED)

# Building an executable
add_executable(champsim_plus_ramulator) # This command creates a target. In CMake lingo, a target is a name the developer gives to a collection of properties

# Specifies compile definitions to use when compiling a given target
if(NOT ENABLE_ASSERT)
    target_compile_definitions(champsim_plus_ramulator
        PRIVATE
        NDEBUG)
endif()

# Specifies include directories (map to the -I compiler flag) to use when compiling a given target
target_include_directories(champsim_plus_ramulator
    PRIVATE
    "${CMAKE_SOURCE_DIR}/include")

# Adds options (compiler flags) to the COMPILE_OPTIONS or INTERFACE_COMPILE_OPTIONS target properties. These options are used when compiling the given target
target_compile_options(champsim_plus_ramulator
    PRIVATE
    ${WarningConfig}
    ${WarningErrorConfig}
    -fdiagnostics-color=always)

# Specify libraries or flags to use when linking a given target and/or its dependents
target_link_libraries(champsim_plus_ramulator
    PRIVATE
    fmt::fmt
    OpenMP::OpenMP_CXX
    LibLZMA::LibLZMA
    BZip2::BZip2
    ZLIB::ZLIB)

# Sets properties on a given target:
# RUNTIME_OUTPUT_DIRECTORY property specifies the directory into which runtime target files should be built
set_target_properties(champsim_plus_ramulator
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Pull in per-directory source lists. Each subdirectory's CMakeLists.txt
# attaches its files to the executable via target_sources().
add_subdirectory(source)
