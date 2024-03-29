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

cmake_minimum_required(VERSION 3.0...3.7)
project(GrainViewer)

include(cmake/utils.cmake)

option(DEV_MOD "Build in dev mode, use share directory from source tree rather than installation tree. This is useful when hacking on shaders." ON)
option(DOWNLOAD_EXAMPLE_DATA "Download additionnal example data" ON)
option(GIT_SUBMODULE "Check submodules during build" ON)

fetch_submodules()
fetch_example_data()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(src)
