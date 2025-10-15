cmake_minimum_required(VERSION 3.15)
project(OrangeSodium VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Require SIMD macro (OS_AVX or OS_SSE)
if(NOT DEFINED SIMD_TYPE)
    message(FATAL_ERROR "SIMD_TYPE must be defined. Set to either AVX or SSE using -DSIMD_TYPE=AVX or -DSIMD_TYPE=SSE")
endif()

if(SIMD_TYPE STREQUAL "AVX")
    add_compile_definitions(OS_AVX)
    message(STATUS "Building with AVX SIMD support")
elseif(SIMD_TYPE STREQUAL "SSE")
    add_compile_definitions(OS_SSE)
    message(STATUS "Building with SSE SIMD support")
else()
    message(FATAL_ERROR "Invalid SIMD_TYPE: ${SIMD_TYPE}. Must be either AVX or SSE")
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Validate build type
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be either Debug or Release")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/third_party
    ${PROJECT_SOURCE_DIR}/third_party/AudioFile
    ${PROJECT_SOURCE_DIR}/third_party/json/include
    ${PROJECT_SOURCE_DIR}/third_party/lua
)

# Third-party library directories based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LUA_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/lua/build/Debug")
    set(AUDIOFILE_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/AudioFile/build/Debug")
    set(JSON_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/json/build/Debug")
else()
    set(LUA_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/lua/build/Release")
    set(AUDIOFILE_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/AudioFile/build/Release")
    set(JSON_LIB_DIR "${PROJECT_SOURCE_DIR}/third_party/json/build/Release")
endif()

# Link directories for pre-built libraries
link_directories(
    ${LUA_LIB_DIR}
    ${AUDIOFILE_LIB_DIR}
    ${JSON_LIB_DIR}
)

# Compiler flags
if(MSVC)
    # MSVC-specific flags
    add_compile_options(/W4)
    if(SIMD_TYPE STREQUAL "AVX")
        add_compile_options(/arch:AVX)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi /MDd)
    else()
        add_compile_options(/O2 /MD)
    endif()
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -pedantic)
    if(SIMD_TYPE STREQUAL "AVX")
        add_compile_options(-mavx)
    elseif(SIMD_TYPE STREQUAL "SSE")
        add_compile_options(-msse -msse2)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Source files
set(SOURCES
    src/orange_sodium.cpp
    src/oscillator.cpp
    src/program.cpp
    src/signal_buffer.cpp
    src/synthesizer.cpp
    src/voice.cpp
    src/effects/simple_distortion.cpp
)

# Create the library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link third-party libraries
# Note: Adjust library names based on actual pre-built library names
# Common naming conventions: liblua.a, lua.lib, lua54.lib, etc.
if(MSVC)
    target_link_libraries(${PROJECT_NAME}
        lua55
        # AudioFile is header-only, no linking needed
        # JSON is header-only, no linking needed
    )
else()
    target_link_libraries(${PROJECT_NAME}
        lua55
        # AudioFile is header-only, no linking needed
        # JSON is header-only, no linking needed
    )
endif()

# Installation rules (optional)
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

# Examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    # Basic example
    add_executable(basic_example examples/basic/main.cpp)
    target_link_libraries(basic_example ${PROJECT_NAME})

    # Set output directory for examples
    set_target_properties(basic_example
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endif()

# Print configuration summary
message(STATUS "================================")
message(STATUS "OrangeSodium Configuration")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SIMD type: ${SIMD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Lua library dir: ${LUA_LIB_DIR}")
message(STATUS "AudioFile library dir: ${AUDIOFILE_LIB_DIR}")
message(STATUS "JSON library dir: ${JSON_LIB_DIR}")
message(STATUS "================================")
