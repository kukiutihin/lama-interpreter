# include(FetchContent)

# asmjit library
# FetchContent_Declare(asmjit
#     GIT_REPOSITORY https://github.com/asmjit/asmjit.git
#     GIT_TAG        master
# )
# FetchContent_MakeAvailable(asmjit)


add_executable(lamaj
  main.cpp
  virtual-machine.cpp
  bytecode-reader.cpp
)

target_compile_options(lamaj PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-writable-strings>
)

target_link_libraries(lamaj PRIVATE 
  runtime
  # asmjit::asmjit
)
