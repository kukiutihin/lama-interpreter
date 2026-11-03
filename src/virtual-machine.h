#pragma once

#include <cstddef>
#include <cstdint>

#include "app-runtime.h"
#include "stack.h"
#include "tokens.h"

class VirtualMachine {
  std::uint8_t* constantPool;
  std::size_t globalsCount;

  // needed to use instead of real ip
  std::uint8_t dummyForEOF[1] = {OP_EOF};
  std::uint8_t* eof = &dummyForEOF[0];

 public:
  VirtualMachine(std::uint8_t* pool, std::size_t globalsCount, const GC& gc)
      : constantPool(pool), globalsCount(globalsCount) {}

  void interpret(uint8_t*, std::size_t);

  Stack allocateStack();
};
