
cmake_minimum_required(VERSION 3.10)

project(webserver CXX)
set(CMAKE_CXX_STANDARD 17)

set(root ${CMAKE_SOURCE_DIR})

# 项目选项
option(BUILD_TESTS "Build tests" OFF)

add_subdirectory(${root}/core)

set(BUILD_TESTS ON)
if(${BUILD_TESTS})
    add_subdirectory(${root}/test)
endif()