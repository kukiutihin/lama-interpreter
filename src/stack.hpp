#include <cstddef>
#include <utility>

#include "runtime.hpp"

static constexpr size_t ELEMSIZE = sizeof(ptrt);
static constexpr size_t STACKSIZE = 1024 * 8;

using std::size_t;

class Stack {
  ptrt* stack;
  size_t current;
  size_t& fp;
  size_t& sp;

 public:
  Stack(size_t& fp, size_t& sp) : fp(fp), sp(sp), current(0) {
    stack = new ptrt[STACKSIZE];
    fp = (size_t)stack;
    sp = fp;
  }

  [[gnu::always_inline]] ptrt pop() {
    sp += ELEMSIZE;
    return stack[--current];
  }

  [[gnu::always_inline]] ptrt top() { return stack[current - 1]; }

  [[gnu::always_inline]] void swap() {
    std::swap(stack[current - 1], stack[current - 1]);
  }

  [[gnu::always_inline]] void popN(std::size_t n) {
    for (std::size_t i = 0; i < n; i++) {
      sp += ELEMSIZE;
      current--;
    }
  }

  [[gnu::always_inline]] void push(ptrt value) {
    sp -= ELEMSIZE;
    stack[current++] = value;
  }

  [[gnu::always_inline]] ptrt* getAddr(std::size_t offsetFromCurrent) {
    return &stack[current - offsetFromCurrent];
  }
};
