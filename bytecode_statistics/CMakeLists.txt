add_executable(lamastat
  main.cpp
)

target_compile_options(lamastat PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-writable-strings>
)

target_link_libraries(lamastat PRIVATE 
  runtime
)
