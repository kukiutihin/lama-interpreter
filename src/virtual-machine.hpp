#include <cstdint>

#include "runtime.hpp"
#include "constant-pool.hpp"

class VirtualMachine {
  ConstantPool& constantPool;
  GC& gc;

 public:
  VirtualMachine(GC& gc, ConstantPool& pool)
      : gc(gc), constantPool(pool) {}

  ptrt interpret(uint8_t*);
};
