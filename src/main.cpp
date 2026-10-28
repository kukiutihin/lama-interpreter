#include <cstring>
#include <print>

#include "runtime.hpp"
#include "tokens.hpp"
#include "virtual-machine.hpp"

int main() {
  GC gc;
  const char* constPool[] = {"moskib", "keks", "skib", "penguinbox"};
  VirtualMachine vm(gc, constPool);

  static uint8_t ir[] = {OP_STRING, 2, 0, 0, 0, OP_END};
  auto res = vm.interpret(ir);
  std::println("{}", reinterpret_cast<char*>(res));
}
