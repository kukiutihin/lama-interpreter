#pragma once

#include <cstddef>

extern "C" {
#include "gc.h"
#include "runtime_common.h"

extern void* Bstring(aint* args);
extern size_t __gc_stack_top;
extern size_t __gc_stack_bottom;
}

class GC {
 public:
  GC(const GC&) = delete;
  GC& operator=(const GC&) = delete;
  GC&& operator=(GC&&) = delete;
  GC(GC&&) = delete;

  GC() { __init(); }
  ~GC() { __shutdown(); }

  template <typename T>
  T* allocate(std::size_t n) {
    return reinterpret_cast<T*>(alloc(n * sizeof(T)));
  }
};
