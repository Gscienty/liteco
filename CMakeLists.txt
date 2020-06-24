cmake_minimum_required(VERSION 2.8.8)

enable_language(ASM)

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

project(liteco)

set (LITECO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)


if (CMAKE_COMPILER_IS_GNUCC) 

    add_compile_options(-Wall)

endif (CMAKE_COMPILER_IS_GNUCC)

set(LITECO_ARCH x86_64)

message("arch: x86_64")

aux_source_directory(${PROJECT_SOURCE_DIR}/src LITECO_SOURCE)

set(LITECO_ASM
    ${PROJECT_SOURCE_DIR}/src/arch/${LITECO_ARCH}/atomic-cas.s
    ${PROJECT_SOURCE_DIR}/src/arch/${LITECO_ARCH}/make.s
    ${PROJECT_SOURCE_DIR}/src/arch/${LITECO_ARCH}/p-yield.s
    ${PROJECT_SOURCE_DIR}/src/arch/${LITECO_ARCH}/swap.s
    )

message("${LITECO_ASM}")

include_directories(${LITECO_INCLUDE})


add_library(liteco SHARED ${LITECO_ASM} ${LITECO_SOURCE})
add_library(liteco_static STATIC ${LITECO_ASM} ${LITECO_SOURCE})

target_link_libraries(liteco -lpthread)
target_link_libraries(liteco_static -lpthread)

set_target_properties(liteco_static PROPERTIES OUTPUT_NAME "liteco")

set_target_properties(liteco PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(liteco_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

