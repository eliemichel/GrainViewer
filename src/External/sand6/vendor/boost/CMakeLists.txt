# Extracted from boost using 'bcp seralization'
cmake_minimum_required(VERSION 3.0)

file(GLOB SRC_filesystem libs/filesystem/src/*.cpp)
file(GLOB SRC_serialization libs/serialization/src/*.cpp)
file(GLOB SRC_system libs/system/src/*.cpp)

set(SRC
	${SRC_filesystem}
	${SRC_serialization}
	${SRC_system}
)

add_library(sand6_boost "${SRC}")
target_include_directories(sand6_boost PUBLIC .)

# TODO: make more flexible...
set_target_properties(sand6_boost PROPERTIES
	OUTPUT_NAME "libboost_serialization-vc141-mt-gd-x64-1_69"
)

set_property(TARGET sand6_boost PROPERTY FOLDER "External/Sand6")
group_source_by_folder(${SRC})
