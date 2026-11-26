add_library(lamastat-core STATIC
  src/bytecode-analyzer.cpp
)

target_include_directories(lamastat-core
  PUBLIC src
)

target_link_libraries(lamastat-core PUBLIC
  include
)

add_executable(lamastat
  src/main.cpp
)

target_compile_options(lamastat PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-writable-strings>
)

target_link_libraries(lamastat PRIVATE 
  lamastat-core 
)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
