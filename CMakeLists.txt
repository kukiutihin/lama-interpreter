cmake_minimum_required(VERSION 3.15)
project(LamaJit VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 
)
FetchContent_MakeAvailable(Catch2)

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

set(VM_DIR ${CMAKE_SOURCE_DIR}/Lama/virtual_machine)
add_library(vm STATIC
  ${VM_DIR}/bytecode.c
  ${VM_DIR}/memory.c
  ${VM_DIR}/opcodes.c
)
target_include_directories(vm PUBLIC
  ${VM_DIR}
)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
add_library(include INTERFACE)
target_include_directories(include INTERFACE
  ${INCLUDE_DIR}
)
target_compile_options(include INTERFACE
  $<$<COMPILE_LANGUAGE:C, CXX>:-Wno-writable-strings>
)
target_link_libraries(include
  INTERFACE vm
)

add_subdirectory(interpreter)
add_subdirectory(bytecode-statistics)
