cmake_minimum_required(VERSION 3.0)

add_subdirectory(boost)

add_library(sand6_vendor INTERFACE)
target_include_directories(sand6_vendor INTERFACE . eigen3)
