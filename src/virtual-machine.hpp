#pragma once

#include <cstddef>
#include <cstdint>

#include "runtime.hpp"
#include "stack.hpp"

class VirtualMachine {
  std::uint8_t* constantPool;
  GC& gc;
  std::size_t globalsCount;

 public:
  VirtualMachine(GC& gc, std::uint8_t* pool, std::size_t globalsCount)
      : gc(gc), constantPool(pool), globalsCount(globalsCount) {}

  void interpret(uint8_t*);

  Stack allocateStack() {
    Stack stack;

    // reserves space for globals in stack
    for (std::size_t i = 0; i < globalsCount; i++) stack.push(BOX(0));
    return stack;
  }
};
