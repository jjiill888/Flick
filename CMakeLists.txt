cmake_minimum_required(VERSION 3.15)
project(LetsCode)

set(CMAKE_CXX_STANDARD 17)

find_package(FLTK REQUIRED NO_MODULE)

add_executable(lets_code
    src/main.cpp
    src/editor_window.cpp
    src/file_tree.cpp
    src/utils.cpp
)

target_link_libraries(lets_code PRIVATE fltk::fltk fltk::forms fltk::images)