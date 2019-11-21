# This file is part of Augen Light
#
# Copyright (c) 2017 - 2019 -- Élie Michel <elie.michel@exppad.com>
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
