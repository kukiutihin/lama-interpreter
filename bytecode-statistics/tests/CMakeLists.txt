add_executable(lamastat-tests
  tests.cpp
)

target_link_libraries(lamastat-tests PRIVATE
  lamastat-core
  Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(lamastat-tests)

