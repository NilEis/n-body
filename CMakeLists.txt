cmake_minimum_required(VERSION 3.9.0)
project(
  n-body
  VERSION 0.1.0
  LANGUAGES C)

option(USE_CUDA "Use cuda" OFF)
option(USE_OPENCL "Use opencl" OFF)
option(USE_COMPUTE_SHADER "Use compute shader" OFF)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-ffunction-sections -fdata-sections -march=native)
endif()
if(NOT EMSCRIPTEN)
  add_subdirectory(extern/glfw)
endif()
add_subdirectory(extern/cglm)

add_subdirectory(extern/logger)

set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/extern/glad/")
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

set(link_list glfw glad_gl_core_33 ma-log cglm)

set(source_files
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vec3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/particle.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/solver.c)

set(different_source_files)

if(USE_OPENCL
  AND NOT USE_CUDA
  AND NOT USE_COMPUTE_SHADER)
  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/opencl_kernel.cl)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/opencl_kernel.cl opencl_kernel_cl_file)
  string(REPLACE "\n" "\\n\"\\\n\"" opencl_kernel_cl "${opencl_kernel_cl_file}")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/opencl_kernel.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opencl_kernel.h @ONLY)
  add_compile_definitions(USE_CUDA=0 USE_OPENCL=1)
  find_package(OpenCL REQUIRED)
  list(APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/opencl_tick.c)
  list(APPEND link_list OpenCL::OpenCL)
endif()
if(USE_CUDA
  AND NOT USE_OPENCL
  AND NOT USE_COMPUTE_SHADER)
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT result
    OUTPUT output
    LANGUAGES C)
  # set(CMAKE_CUDA_COMPILER clang++)
  option(USE_OCTREE "Use an octree implementation" FALSE)
  enable_language(CUDA)
  add_compile_definitions(USE_CUDA=1 USE_OPENCL=0 USE_COMPUTE_SHADER=0)
  list(APPEND different_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda_tick.cu)
  if(USE_OCTREE)
    list(APPEND different_source_files
      ${CMAKE_CURRENT_SOURCE_DIR}/src/octree.cu)
    add_compile_definitions(USE_OCTREE=1)
  endif()
else()
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT result
    OUTPUT output
    LANGUAGES C)
endif()

if(USE_COMPUTE_SHADER
  AND NOT USE_CUDA
  AND NOT USE_OPENCL)
  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/compute_shader_solve.comp)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shader/compute_shader_solve.comp
    compute_shader_solve_comp_file)
  string(REPLACE "\n" "\\n\"\\\n\"" compute_shader_solve_comp
    "${compute_shader_solve_comp_file}")

  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/compute_shader_update.comp)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shader/compute_shader_update.comp
    compute_shader_update_comp_file)
  string(REPLACE "\n" "\\n\"\\\n\"" compute_shader_update_comp
    "${compute_shader_update_comp_file}")
  add_compile_definitions(USE_COMPUTE_SHADER=1 USE_CUDA=0 USE_OPENCL=0)
endif()

if(NOT USE_OPENCL
  AND NOT USE_CUDA
  AND NOT USE_COMPUTE_SHADER)
  find_package(OpenMP)
  if(OpenMP_C_FOUND)
    message(INFO " Use OpenMP with ${OpenMP_C_FLAGS}")
    add_compile_options(${OpenMP_C_FLAGS})
    add_definitions(-DUSE_OPENMP)
    list(APPEND link_list OpenMP::OpenMP_C)
  endif()
endif()

if(EMSCRIPTEN)
  if(NOT USE_OPENCL AND NOT USE_CUDA)
    set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 -gsource-map"
    )
    set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/shell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Remove the -rdynamic flag because otherwise emscripten does not generate
    # HTML+JS+WASM files, only a non-working and fat HTML
    string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS
      "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
  else()
    message(ERROR "Only shader and threading are supported")
  endif()
endif()

add_executable(n-body ${source_files} ${different_source_files})

set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shader/main.fs)
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shader/main.vs)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shader/main.fs SHADER_fs_file)
string(REPLACE "\n" "\\n\"\\\n\"" SHADER_fs "${SHADER_fs_file}")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/shader/main.vs SHADER_vs_file)
string(REPLACE "\n" "\\n\"\\\n\"" SHADER_vs "${SHADER_vs_file}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/shader.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/shader.h @ONLY)

file(GLOB_RECURSE header_files ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

target_include_directories(n-body PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_property(TARGET n-body PROPERTY C_STANDARD 11)
set_target_properties(n-body PROPERTIES CUDA_ARCHITECTURES "native")
target_link_libraries(n-body ${link_list})
list(LENGTH source_files batch_size)
set_target_properties(${name} PROPERTIES
  UNITY_BUILD TRUE
  UNITY_BUILD_MODE BATCH
  UNITY_BUILD_BATCH_SIZE ${batch_size}
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_link_options(n-body PRIVATE -Wl,--gc-sections)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_definitions(USE_PTHREAD=0)
endif()

if(result)
  message(STATUS "IPO is supported")
  set_property(TARGET n-body PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported: ${output}")
endif()
