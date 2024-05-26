cmake_minimum_required(VERSION 3.10)

# Project name and version
project(plume-vm VERSION 1.0)

# Set the toolchain based on the platform
if (WIN32)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
else()
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang)
endif()

# Set warning level
add_compile_options(-Wall -Wextra -Wpedantic)

# Glob source files recursively
file(GLOB_RECURSE SRC_FILES "src/*.c")

# Add the include directory
include_directories(include)

# Define the plume-vm target
add_executable(plume-vm ${SRC_FILES})
set_target_properties(plume-vm PROPERTIES OUTPUT_NAME "plume-vm")
set_target_properties(plume-vm PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set optimization level for release mode
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

# Add include directories to the plume-vm target
target_include_directories(plume-vm PRIVATE include)

# Define the plume-vm-test target
add_executable(plume-vm-test ${SRC_FILES})
set_target_properties(plume-vm-test PROPERTIES OUTPUT_NAME "plume-vm-test")
set_target_properties(plume-vm-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set flags and optimization level for debug and profile modes
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pg")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -pg")

# Enable debug symbols and profiling flags for plume-vm-test
target_compile_options(plume-vm-test PRIVATE -g)
target_link_options(plume-vm-test PRIVATE -pg)

# Add include directories to the plume-vm-test target
target_include_directories(plume-vm-test PRIVATE include)

# Add a shared library target version

# Define the plume-runtime-lib shared library
add_library(plume-runtime-lib STATIC ${SRC_FILES})
set_target_properties(plume-runtime-lib PROPERTIES OUTPUT_NAME "plume-runtime-lib")

# Set the output directory for the shared library
set_target_properties(plume-runtime-lib PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add include directories to the plume-runtime-lib target
target_include_directories(plume-runtime-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
