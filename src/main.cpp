#include <cstdint>
#include <cstring>
#include <print>

#include "runtime.hpp"
#include "tokens.hpp"
#include "virtual-machine.hpp"

int main() {
  GC gc;
  std::uint8_t constPool[] = {CONST_STRING, 's', 'k', 'i', 'b', '\0',
                              CONST_STRING, 'K', 'e', 'k', 's', '\0'};
  auto constPoolRef = constPool;

  VirtualMachine vm(gc, constPoolRef);

  static uint8_t ir[] = {OP_CONST, 9,     0, 0, 0, OP_CONST,  5,  0, 0, 0,
                         OP_CONST, 13,    0, 0, 0, OP_BARRAY, 3,  0, 0, 0,
                         OP_CONST, 1,     0, 0, 0, OP_CONST,  87, 0, 0, 0,
                         OP_STA, OP_DROP, OP_EOF};

  auto res = vm.interpret(ir);
  std::println("{}", res);
}
