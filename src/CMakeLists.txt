cmake_minimum_required(VERSION 2.6)
project(Duplicate_Files_Remover)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include(FindBoost)
find_package(
    Boost
    1.54.0
    REQUIRED
    COMPONENTS
    filesystem
    system
)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(
  GLOB_RECURSE
  source_files
  src/*
)

add_executable(Duplicate_Files_Remover main.cpp)

target_link_libraries(Duplicate_Files_Remover ${Boost_LIBRARIES})

