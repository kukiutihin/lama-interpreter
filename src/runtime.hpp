#pragma once

#include <cstddef>

extern "C" {
#include "gc.h"
#include "runtime_common.h"

extern void* Bstring(aint* args);
}

class Runtime {
 public:
  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;
  Runtime&& operator=(Runtime&&) = delete;
  Runtime(Runtime&&) = delete;

  Runtime() { __init(); }
  ~Runtime() { __shutdown(); }

  template <typename T>
  T* allocate(std::size_t n) {
    return reinterpret_cast<T*>(alloc(n * sizeof(T)));
  }
};