cmake_minimum_required(VERSION 3.0)
project(tinygltf)

set(SRC
	include/tiny_gltf.h
	include/json.hpp
	include/stb_image.h
	include/stb_image_write.h
	src/tiny_gltf.cpp
)

add_library(tinygltf ${SRC})
target_include_directories(tinygltf PUBLIC include/)
target_compile_definitions(
	tinygltf PUBLIC
	-DTINYGLTF_USE_CPP14
	-DTINYGLTF_NO_STB_IMAGE_WRITE
)
set_property(TARGET tinygltf PROPERTY FOLDER "External")
