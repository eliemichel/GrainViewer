cmake_minimum_required(VERSION 3.0)

OPTION( OPENMP  "Use OpenMP" ON )
if( OPENMP )
    FIND_PACKAGE(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        add_definitions(-DEIGEN_DONT_PARALLELIZE)
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas " )
    endif()
endif()

file(GLOB_RECURSE SRC *.cc)
file(GLOB_RECURSE HDR *.hh)

add_library(sand6 "${SRC}" "${HDR}")
target_include_directories(sand6 PUBLIC . 3d)
target_link_libraries(sand6 PUBLIC sand6_vendor sand6_boost)

target_compile_definitions(sand6 PRIVATE
	"-DD6_DIM=3"
	"-DNOMINMAX"
	"-DBOGUS_WITH_BOOST_SERIALIZATION"
)

target_compile_options(sand6 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)


set_property(TARGET sand6 PROPERTY FOLDER "External/Sand6")
group_source_by_folder(${SRC} ${HDR})
