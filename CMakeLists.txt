cmake_minimum_required(VERSION 3.15)
project(LetsCode VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_DEBUG "Enable debug mode" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

# Set compilation flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR ENABLE_DEBUG)
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic)
    add_definitions(-DDEBUG)
else()
    # Optimization compilation options
    add_compile_options(-O3 -DNDEBUG -march=native -mtune=native)
    
    # Enable Link Time Optimization
    if(ENABLE_LTO)
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
    
    # Other optimization options
    add_compile_options(-ffast-math -funroll-loops -fomit-frame-pointer)
    
    # Reduce binary size
    add_compile_options(-fdata-sections -ffunction-sections)
    add_link_options(-Wl,--gc-sections)
endif()

# Set linker optimizations
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Static link standard libraries to reduce startup time
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    
    # Optimize symbol table
    add_link_options(-Wl,--strip-all)
endif()

# Find FLTK
find_package(FLTK REQUIRED NO_MODULE)

# Include directories
include_directories(${FLTK_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/editor_window.cpp
    src/file_tree.cpp
    src/utils.cpp
    src/SearchReplace.cpp
    src/scrollbar_theme.cpp
    src/editor_state.cpp
    src/tab_bar.cpp
)

# Header files
set(HEADERS
    src/editor_window.hpp
    src/file_tree.hpp
    src/utils.hpp
    src/globals.hpp
    src/SearchReplace.hpp
    src/scrollbar_theme.hpp
    src/editor_state.hpp
    src/tab_bar.hpp
)

# Create executable
add_executable(lets_code ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(lets_code PRIVATE
    fltk::fltk 
    fltk::forms 
    fltk::images
)

# Set output directory
set_target_properties(lets_code PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install rules
install(TARGETS lets_code
    RUNTIME DESTINATION bin
)

# Install resource files
install(FILES logo.png
    DESTINATION share/lets_code
)

# If tests are enabled
if(BUILD_TESTS)
    enable_testing()
    # Test-related configuration can be added here
endif()

# Print configuration information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "FLTK version: ${FLTK_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
