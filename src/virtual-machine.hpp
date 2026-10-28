#include <cstdint>

#include "runtime.hpp"

class VirtualMachine {
  const char** constantPool;
  Runtime& runtime;

 public:
  VirtualMachine(Runtime& runtime, const char** pool)
      : runtime(runtime), constantPool(pool) {}

  ptrt interpret(uint8_t*);
};