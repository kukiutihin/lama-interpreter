#pragma once

#include <cstddef>

extern "C" {
#include "gc.h"
#include "runtime_common.h"

extern aint Ls__Infix_43(void* p, void* q);    // +
extern aint Ls__Infix_45(void* p, void* q);    // -
extern aint Ls__Infix_42(void* p, void* q);    // *
extern aint Ls__Infix_47(void* p, void* q);    // /
extern aint Ls__Infix_37(void* p, void* q);    // %
extern aint Ls__Infix_3333(void* p, void* q);  // !!
extern aint Ls__Infix_3838(void* p, void* q);  // &&
extern aint Ls__Infix_60(void* p, void* q);    // <
extern aint Ls__Infix_6061(void* p, void* q);  // <=
extern aint Ls__Infix_62(void* p, void* q);    // >
extern aint Ls__Infix_6261(void* p, void* q);  // >=
extern aint Ls__Infix_6161(void* p, void* q);  // ==
extern aint Ls__Infix_3361(void* p, void* q);  // !=

extern void* Bstring(aint* args);
extern void* Bsexp(aint* args, aint n);
extern void* Bsta(void* x, aint i, void* v);
extern void* Barray(aint* args, aint bn);
extern void* Belem(void* p, aint i);
extern void* Bclosure(aint* args, aint bn);

extern aint Btag(void* d, aint t, aint n);
extern aint Barray_patt(void* d, aint n);
extern aint Bstring_patt(void* x, void* y);
extern aint Bclosure_tag_patt(void* x);
extern aint Barray_tag_patt(void* x);
extern aint Bstring_tag_patt(void* x);
extern aint Bsexp_tag_patt(void* x);
extern aint Bboxed_patt(void* x);
extern aint Bunboxed_patt(void* x);

extern _Noreturn void failure(char* s, ...);

extern aint Lwrite(aint n);
extern aint Lread();
extern aint LtagHash(const char* s);
extern int Llength(void* p);
extern void* Lstring (aint *p);

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
