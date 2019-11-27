cmake_minimum_required(VERSION 3.1)
project(libSOIL)

#############################

# Disable warnings

if (MSVC)
	add_definitions(-W0)
else()
	add_definitions(-w)
endif()


#############################

add_library(libSOIL
	image_helper.h
	image_helper.c
	stb_image_aug.h
	stb_image_aug.c
	image_DXT.h
	image_DXT.c
	SOIL.c
	SOIL.h
	stbi_DDS_aug.h
	stbi_DDS_aug_c.h
)

target_include_directories(libSOIL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(CMAKE_COMPILER_IS_GNUCC)
	target_link_libraries(libSOIL LINK_PRIVATE GL)
endif(CMAKE_COMPILER_IS_GNUCC)
set_property(TARGET libSOIL PROPERTY FOLDER "External")
