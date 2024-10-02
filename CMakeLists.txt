cmake_minimum_required(VERSION 3.9.0)
project(
        n-body
        VERSION 0.1.0
        LANGUAGES C)

set(GLFW_USE_HYBRID_HPG TRUE)

file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
        ${CMAKE_BINARY_DIR}/CPM.cmake)
include(${CMAKE_BINARY_DIR}/CPM.cmake)

cpmaddpackage("gh:glfw/glfw#master")
cpmaddpackage(
        NAME
        glad
        GITHUB_REPOSITORY
        Dav1dde/glad
        GIT_TAG
        glad2
        DOWNLOAD_ONLY
        YES)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed.cmake)

create_resources(
        ${CMAKE_CURRENT_SOURCE_DIR}/shader "shader_"
        ${CMAKE_CURRENT_BINARY_DIR}/generated/src/shader.c
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include/shader.h)

add_subdirectory("${glad_SOURCE_DIR}/cmake" "${glad_BINARY_DIR}")

glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

file(GLOB N-BODY_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

list(APPEND N-BODY_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/src/shader.c)

add_executable(n-body ${N-BODY_SOURCE_FILES})

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shader)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shader)

file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_BINARY_DIR}/generated/include/*.h)

target_precompile_headers(n-body PRIVATE ${HEADER_FILES})

target_include_directories(n-body PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_BINARY_DIR}/generated/include/)

target_link_libraries(n-body glfw glad_gl_core_46)

include(CheckIPOSupported)

check_ipo_supported(
        RESULT result
        OUTPUT output
        LANGUAGES C)
if (result)
    message(STATUS "IPO is supported")
    set_property(TARGET n-body PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(WARNING "IPO is not supported: ${output}")
endif ()

if (EMSCRIPTEN)
    # set(SUPPORT_CLANGD OFF CACHE BOOL "" FORCE)
    set(PLATFORM
            "Web"
            CACHE STRING "" FORCE)
    # remove --closure-args="--compilation_level SIMPLE" when
    # https://github.com/emscripten-core/emscripten/issues/16706 is fixed
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --no-heap-copy")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(LINK_FLAGS CMAKE_EXE_LINKER_FLAGS)

    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sGL_ENABLE_GET_PROC_ADDRESS")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMINIFY_HTML=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sENVIRONMENT=web")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sFILESYSTEM=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sALLOW_MEMORY_GROWTH=1")
    set(${LINK_FLAGS}
            "${${LINK_FLAGS}} --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/pre.js"
    )
    set(${LINK_FLAGS} "${${LINK_FLAGS}} --closure 0")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMODULARIZE")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -flto")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sEXPORT_NAME=PacMan")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sINVOKE_RUN=1")
    # set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM_ASYNC_COMPILATION")
    set(${LINK_FLAGS}
            "${${LINK_FLAGS}} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html"
    )

    # Remove the -rdynamic flag because otherwise emscripten does not generate
    # HTML+JS+WASM files, only a non-working and fat HTML
    string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS
            "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
endif ()


include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/create_clangd.cmake)
