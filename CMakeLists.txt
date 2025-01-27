cmake_minimum_required(VERSION 3.10)

project(demo)

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# 查找include目录及其子目录中的所有.h文件
file(GLOB_RECURSE HEADER_FILES "include/*.h")

# 查找src目录及其子目录中的所有.cpp文件
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.cc" "src/*.rl" "src/*.rl.cc")

# 将所有文件设置为源文件列表
set(SRC_LIST ${SOURCE_FILES} ${HEADER_FILES})

# 指定 include 目录
include_directories(include)

# 添加可执行文件
add_executable(main ${SRC_LIST})