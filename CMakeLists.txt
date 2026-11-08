cmake_minimum_required(VERSION 3.15)
project(LamaJit VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# asmjit library
FetchContent_Declare(asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(asmjit)


set(RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lama/runtime)
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

add_executable(lamaj
  ${SRC}/main.cpp
  ${SRC}/virtual-machine.cpp
  ${SRC}/bytecode-reader.cpp
)

target_compile_options(lamaj PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-writable-strings>
)

target_include_directories(lamaj PRIVATE
  ${SRC}
)

target_link_libraries(lamaj PRIVATE 
  runtime
  asmjit::asmjit
)
