#include <cstdint>
#include <cstring>

#include "runtime.hpp"
#include "tokens.hpp"
#include "virtual-machine.hpp"

int main() {
  GC gc;
  std::uint8_t constPool[] = {'s', 'k', 'i', 'b', '\0',
                              'K', 'e', 'k', 's', '\0',
                              'w', 'r', 'i', 't', 'e', '\0',
                             };

  VirtualMachine vm(gc, constPool, 1);

  vm.interpret(clo_in_func);
}
