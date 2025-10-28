cmake_minimum_required(VERSION 3.0)
project(gameserver)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")
set(LIBRARY_LINK "spdlog::spdlog fmt::fmt")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
