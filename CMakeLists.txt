cmake_minimum_required(VERSION 3.10)
project(LetsCode)

set(CMAKE_CXX_STANDARD 17)

# 获取 fltk-config 的参数
execute_process(
  COMMAND fltk-config --cxxflags
  OUTPUT_VARIABLE FLTK_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND fltk-config --ldflags
  OUTPUT_VARIABLE FLTK_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 所有源文件
add_executable(lets_code
    src/main.cpp
    src/editor_window.cpp
    src/file_tree.cpp
    src/utils.cpp
)

# 添加编译和链接选项
target_compile_options(lets_code PRIVATE ${FLTK_CXXFLAGS})
target_link_options(lets_code PRIVATE ${FLTK_LDFLAGS})