cmake_minimum_required(VERSION 3.0)
project(tinypngout)

add_library(tinypngout include/TinyPngOut.hpp src/TinyPngOut.cpp)
target_include_directories(tinypngout PUBLIC include/)
set_property(TARGET tinypngout PROPERTY FOLDER "External")
