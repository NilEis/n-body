cmake_minimum_required(VERSION 3.9.0)
project(n-body VERSION 0.1.0 LANGUAGES C)

option(USE_CUDA "Use cuda" ON)


if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
add_compile_options(-ffunction-sections -fdata-sections -march=native)
endif()

add_subdirectory(extern/glfw)

add_subdirectory(extern/cglm)

add_subdirectory(extern/logger)

set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/extern/glad/")
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

if(USE_CUDA)
    enable_language(CUDA)
    add_compile_definitions(USE_CUDA=1)
    add_executable(n-body ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/octree.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/particle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda_tick.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver.c)
else()
    add_executable(n-body ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/octree.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/particle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver.c)

endif()

set_property(TARGET n-body PROPERTY C_STANDARD 11)
set_target_properties(n-body PROPERTIES CUDA_ARCHITECTURES "all")

target_include_directories(n-body PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shader/main.fs)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shader/main.vs)
file(READ ${CMAKE_SOURCE_DIR}/shader/main.fs SHADER_fs_file)
string(REPLACE "\n" "\\n\"\\\n\"" SHADER_fs "${SHADER_fs_file}")
file(READ ${CMAKE_SOURCE_DIR}/shader/main.vs SHADER_vs_file)
string(REPLACE "\n" "\\n\"\\\n\"" SHADER_vs "${SHADER_vs_file}")

configure_file(${CMAKE_SOURCE_DIR}/include/shader.h.in ${CMAKE_SOURCE_DIR}/include/shader.h @ONLY)

target_link_libraries(n-body glfw glad_gl_core_33 ma-log cglm)

include(CheckIPOSupported)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_link_options(n-body PRIVATE -Wl,--gc-sections)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
add_compile_definitions(USE_PTHREAD=0)
endif()

check_ipo_supported(RESULT result OUTPUT output LANGUAGES C)
if(result)
message(STATUS "IPO is supported")
set_property(TARGET n-body PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
message(WARNING "IPO is not supported: ${output}")
endif()
