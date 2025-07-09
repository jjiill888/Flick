cmake_minimum_required(VERSION 3.15)
project(LetsCode VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_DEBUG "Enable debug mode" OFF)

# 设置编译标志
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR ENABLE_DEBUG)
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic)
    add_definitions(-DDEBUG)
else()
    add_compile_options(-O2 -DNDEBUG)
endif()

# 查找FLTK
find_package(FLTK REQUIRED NO_MODULE)

# 包含目录
include_directories(${FLTK_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

# 源文件
set(SOURCES
    src/main.cpp
    src/editor_window.cpp
    src/file_tree.cpp
    src/utils.cpp
    src/SearchReplace.cpp
    src/scrollbar_theme.cpp
    src/editor_state.cpp
)

# 头文件
set(HEADERS
    src/editor_window.hpp
    src/file_tree.hpp
    src/utils.hpp
    src/globals.hpp
    src/SearchReplace.hpp
    src/scrollbar_theme.hpp
    src/editor_state.hpp
)

# 创建可执行文件
add_executable(lets_code ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(lets_code PRIVATE 
    fltk::fltk 
    fltk::forms 
    fltk::images
)

# 设置输出目录
set_target_properties(lets_code PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS lets_code
    RUNTIME DESTINATION bin
)

# 安装资源文件
install(FILES logo.png
    DESTINATION share/lets_code
)

# 如果启用测试
if(BUILD_TESTS)
    enable_testing()
    # 这里可以添加测试相关的配置
endif()

# 打印配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "FLTK version: ${FLTK_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
