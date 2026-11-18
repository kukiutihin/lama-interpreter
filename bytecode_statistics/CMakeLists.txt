add_executable(lamastat
  main.cpp
  bytecode-analyzer.cpp
)

target_compile_options(lamastat PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-writable-strings>
)

target_link_libraries(lamastat PRIVATE 
  runtime
  include
)
