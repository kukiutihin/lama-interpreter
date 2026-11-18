cmake_minimum_required(VERSION 3.15)
project(LamaJit VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/Lama/runtime)
add_library(runtime STATIC
  ${RUNTIME_DIR}/runtime.c 
  ${RUNTIME_DIR}/gc.c
)
target_compile_options(runtime PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-Wno-shift-negative-value>
  $<$<COMPILE_LANGUAGE:C>:-Wno-shift-count-overflow>
  $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-to-int-cast>
  $<$<COMPILE_LANGUAGE:C>:-Wno-int-to-pointer-cast>
  $<$<COMPILE_LANGUAGE:C>:-Wno-writable-strings>
)
target_include_directories(runtime PUBLIC
    ${RUNTIME_DIR}
)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
add_library(include INTERFACE)
target_include_directories(include INTERFACE
  ${INCLUDE_DIR}
)

add_subdirectory(interpreter)
add_subdirectory(bytecode-statistics)
