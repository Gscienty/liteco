cmake_minimum_required(VERSION 2.8.8)

project(liteco)

set (LITECO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CMAKE_COMPILER_IS_GNUCC) 

    add_compile_options(-Wall)

endif (CMAKE_COMPILER_IS_GNUCC)

if (CMAKE_CL_64)
    
    set(LITECO_ARCH x86_64)

endif (CMAKE_CL_64)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/arch/${LITECO_ARCH} LITECO_ASM)
aux_source_directory(${PROJECT_SOURCE_DIR}/src LITECO_SOURCE)

include_directories(${LITECO_INCLUDE})

add_library(liteco SHARED ${LITECO_ASM} ${LITECO_SOURCE})
