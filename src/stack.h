#pragma once

#include <cstddef>
#include <utility>

#include "app-runtime.h"

static constexpr size_t ELEMSIZE = sizeof(ptrt);
static constexpr size_t STACKSIZE = 1024 * ELEMSIZE;

class Stack {
  ptrt stack[STACKSIZE];

 public:
  Stack() {
    __gc_stack_top = reinterpret_cast<std::size_t>(&stack[0]);
    __gc_stack_bottom = __gc_stack_top;
  }

  [[gnu::always_inline]] ptrt pop() {
    __gc_stack_bottom -= ELEMSIZE;
    return *reinterpret_cast<ptrt*>(__gc_stack_bottom);
  }

  [[gnu::always_inline]] ptrt top() {
    return *reinterpret_cast<ptrt*>(__gc_stack_bottom - ELEMSIZE);
  }

  [[gnu::always_inline]] void swap() {
    std::swap(*reinterpret_cast<ptrt*>(__gc_stack_bottom - ELEMSIZE),
              *reinterpret_cast<ptrt*>(__gc_stack_bottom - (ELEMSIZE * 2)));
  }

  [[gnu::always_inline]] void popN(std::size_t n) {
    __gc_stack_bottom -= (ELEMSIZE * n);
  }

  [[gnu::always_inline]] void jumpTo(std::size_t offsetEl) {
    __gc_stack_bottom = __gc_stack_top + (offsetEl * ELEMSIZE);
  }

  [[gnu::always_inline]] void push(ptrt value) {
    *reinterpret_cast<ptrt*>(__gc_stack_bottom) = value;
    __gc_stack_bottom += ELEMSIZE;
  }

  [[gnu::always_inline]] std::size_t offset() {
    return ((__gc_stack_bottom - __gc_stack_top) / ELEMSIZE);
  }

  // Returns pointer to an element by given offset (in elements)
  [[gnu::always_inline]] ptrt* byOffset(std::size_t offset) {
    return reinterpret_cast<ptrt*>((__gc_stack_top + (ELEMSIZE * offset)));
  }
};
