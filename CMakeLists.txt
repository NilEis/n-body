cmake_minimum_required(VERSION 3.9.0)
project(n-body VERSION 0.1.0 LANGUAGES C)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-ffunction-sections -fdata-sections -march=native)
endif()

add_subdirectory(extern/glfw)
add_subdirectory(extern/glew)

add_executable(n-body ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
${CMAKE_CURRENT_SOURCE_DIR}/src/octree.c
${CMAKE_CURRENT_SOURCE_DIR}/src/vec3.c
${CMAKE_CURRENT_SOURCE_DIR}/src/particle.c
${CMAKE_CURRENT_SOURCE_DIR}/src/solver.c)

target_include_directories(n-body PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_link_libraries(n-body m glfw libglew_static)

include(CheckIPOSupported)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(n-body PRIVATE -Wl,--gc-sections)
endif()

check_ipo_supported(RESULT result OUTPUT output LANGUAGES C)
if(result)
message(STATUS "IPO is supported")
set_property(TARGET n-body PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
message(WARNING "IPO is not supported: ${output}")
endif()
