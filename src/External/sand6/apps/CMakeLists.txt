cmake_minimum_required(VERSION 3.0)

add_executable(d6 d6.cc d6_version.cc)
target_link_libraries(d6 PRIVATE sand6)
set_property(TARGET d6 PROPERTY FOLDER "External/Sand6")

add_executable(d62vtk d62vtk.cc)
target_link_libraries(d62vtk PRIVATE sand6)
set_property(TARGET d62vtk PROPERTY FOLDER "External/Sand6")

add_executable(d6_solvePrimal d6_solvePrimal.cc)
target_link_libraries(d6_solvePrimal PRIVATE sand6)
set_property(TARGET d6_solvePrimal PROPERTY FOLDER "External/Sand6")

add_executable(d6_analyze d6_analyze.cc)
target_link_libraries(d6_analyze PRIVATE sand6)
set_property(TARGET d6_analyze PROPERTY FOLDER "External/Sand6")
