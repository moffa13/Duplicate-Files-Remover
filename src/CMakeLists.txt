cmake_minimum_required(VERSION 2.6)
project(Duplicates_Files_Remover)

set (CMAKE_CXX_STANDARD 11)

include(FindBoost)
find_package(
    Boost
    1.54.0
    REQUIRED
)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(
  GLOB_RECURSE
  source_files
  src/*
)

add_executable(Duplicates_Files_Remover main.cpp)


