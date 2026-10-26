#include <cstddef>

extern "C" {
#include "gc.h"

int Lbinoperror(void);
}

class Runtime {
 public:
  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;
  Runtime&& operator=(Runtime&&) = delete;
  Runtime(Runtime&&) = delete;

  Runtime() { __init(); }
  ~Runtime() { __shutdown(); }

  void* allocate(std::size_t size) { return alloc(size); }

  void error() { Lbinoperror(); }
};