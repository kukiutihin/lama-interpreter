#include <cstdint>

#include "runtime.hpp"

class VirtualMachine {
  const char** constantPool;
  GC& gc;

 public:
  VirtualMachine(GC& gc, const char** pool)
      : gc(gc), constantPool(pool) {}

  ptrt interpret(uint8_t*);
};
