#include <cstddef>

#include "runtime.hpp"

static constexpr size_t ELEMSIZE = sizeof(ptrt);
static constexpr size_t STACKSIZE = 1024 * 8;

using std::size_t;

class Stack {
  ptrt* stack;
  size_t current;
  size_t* fp;
  size_t* sp;

 public:
  Stack(size_t* fp, size_t* sp) : fp(fp), sp(sp), current(0) {
    stack = new ptrt[STACKSIZE];
    *fp = (size_t)stack;
    *sp = *fp;
  }

  inline ptrt pop() {
    *sp += ELEMSIZE;
    return stack[--current];
  }

  inline void push(ptrt value) {
    *sp -= ELEMSIZE;
    stack[current++] = value;
  }
};
