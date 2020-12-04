# This file is part of GrainViewer, the reference implementation of:
#
#   Michel, Élie and Boubekeur, Tamy (2020).
#   Real Time Multiscale Rendering of Dense Dynamic Stackings,
#   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
#   https://doi.org/10.1111/cgf.14135
#
# Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided “as is”, without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

# Copy dll in output directory
function(target_link_libraries_and_dll target public_or_private lib)
	target_link_libraries(${target} ${public_or_private} ${lib})
	add_custom_command(
		TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:${lib}>
			$<TARGET_FILE_DIR:${target}>
	)
endfunction()

# Reproduce the original folder layout in IDE
function(group_source_by_folder)
	foreach(file ${ARGV}) 
		# Get the directory of the source file
		get_filename_component(parent_dir "${file}" DIRECTORY)

		# Remove common directory prefix to make the group
		string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" group "${parent_dir}")

		# Make sure we are using windows slashes
		string(REPLACE "/" "\\" group "${group}")

		# Group into "Source Files" and "Header Files"
		if ("${file}" MATCHES ".*\\.cpp")
		   set(group "Source Files\\${group}")
		elseif("${file}" MATCHES ".*\\.h")
		   set(group "Header Files\\${group}")
		endif()

		source_group("${group}" FILES "${file}")
	endforeach()
endfunction()

macro(enable_multiprocessor_compilation)
	if(MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
	endif(MSVC)
endmacro()

macro(enable_cpp17)
	set(CMAKE_CXX_STANDARD 17)
	if(MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
	endif(MSVC)
endmacro()

function(target_treat_warnings_as_errors target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /W4 /WX)
	else()
		target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic -Werror)
	endif()
endfunction()

# From https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
macro(fetch_submodules)
	find_package(Git QUIET)
	if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/External/glad/CMakeLists.txt")
		if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
		# Update submodules as needed
		
			if(GIT_SUBMODULE)
				message(STATUS "Submodule update")
				execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
								WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								RESULT_VARIABLE GIT_SUBMOD_RESULT)
				if(NOT GIT_SUBMOD_RESULT EQUAL "0")
					message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
				endif()
			endif()
		endif()

		if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/External/glad/CMakeLists.txt")
			message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
		endif()
	endif()
endmacro()

macro(fetch_example_data)
	if(DOWNLOAD_EXAMPLE_DATA AND NOT EXISTS "${PROJECT_SOURCE_DIR}/share/scenes/Textures")
		message(STATUS "Downloading minimal additionnal example data (run download-full-data.sh/bat manually for extra examples)...")
		if(WIN32)
		    set(DOWNLOAD_SCRIPT "${PROJECT_SOURCE_DIR}/download-minimal-data.bat")
		else()
			set(DOWNLOAD_SCRIPT sh ${PROJECT_SOURCE_DIR}/download-minimal-data.sh)
		endif()
		message(STATUS "running ${DOWNLOAD_SCRIPT}...")
		execute_process(COMMAND ${DOWNLOAD_SCRIPT}
						WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
						RESULT_VARIABLE DOWNLOAD_SCRIPT_RESULT)
		if(NOT DOWNLOAD_SCRIPT_RESULT EQUAL "0")
			message(FATAL_ERROR "download-data script failed with ${DOWNLOAD_SCRIPT_RESULT}")
		endif()
	endif()
endmacro()

function(target_set_default_command_line target arguments)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${target}.vcxproj.user"
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<Project ToolsVersion=\"16.0\">\n"
			"  <PropertyGroup>\n"
			"    <LocalDebuggerCommandArguments>${arguments}</LocalDebuggerCommandArguments>\n"
			"    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>\n"
			"  </PropertyGroup>\n"
			"</Project>")
	endif()
endfunction()
