#include <cstring>
#include <print>

#include "runtime.hpp"

int main() {
  Runtime runtime;
  char* str = reinterpret_cast<char*>(runtime.allocate(3));
  std::string_view msg = "Hello Skib and Keks!";
  std::memcpy(str, msg.data(), msg.length());
  std::println("{}", str);

  runtime.error();
}