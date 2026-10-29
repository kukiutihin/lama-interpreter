#include "virtual-machine.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "runtime.hpp"
#include "runtime_common.h"
#include "stack.hpp"
#include "tokens.hpp"

static constexpr std::size_t CONST_SIZE = sizeof(std::int32_t);

#define ASSIGN(op, label) handlers[(op)] = &&label;
#define NEXT goto* handlers[*ip++];

#define BINOP(op)                        \
  auto b = stack.pop();                  \
  auto a = stack.pop();                  \
  stack.push(BOX(UNBOX(a) op UNBOX(b))); \
  NEXT;

ptrt VirtualMachine::interpret(uint8_t* ip) {
  static void* handlers[45];
  Stack stack(__gc_stack_top, __gc_stack_bottom);

  auto read = [&ip]<typename T> [[gnu::always_inline]] (std::size_t size) -> T {
    T var;
    std::memcpy(&var, ip, size);
    ip += size;
    return var;
  };

  auto readI32 = [&] [[gnu::always_inline]] () {
    return read.operator()<std::int32_t>(4);
  };

  auto fromPool = [&] [[gnu::always_inline]] () {
    return &constantPool[readI32() + 1];
  };

  ASSIGN(OP_BINOP_ADD, _add);
  ASSIGN(OP_BINOP_SUB, _sub);
  ASSIGN(OP_BINOP_MUL, _mul);
  ASSIGN(OP_BINOP_DIV, _div);
  ASSIGN(OP_BINOP_MOD, _mod);
  ASSIGN(OP_BINOP_LT, _lt);
  ASSIGN(OP_BINOP_LE, _le);
  ASSIGN(OP_BINOP_GT, _gt);
  ASSIGN(OP_BINOP_GE, _ge);
  ASSIGN(OP_BINOP_EQ, _eq);
  ASSIGN(OP_BINOP_NEQ, _neq);
  ASSIGN(OP_BINOP_AND, _and);
  ASSIGN(OP_BINOP_OR, _or);

  ASSIGN(OP_CONST, _const);
  ASSIGN(OP_STRING, _string);
  ASSIGN(OP_SEXP, _sexp);
  ASSIGN(OP_STA, _sta);
  ASSIGN(OP_JMP, _jmp);
  ASSIGN(OP_END, _end);
  ASSIGN(OP_DROP, _drop);
  ASSIGN(OP_DUP, _dup);
  ASSIGN(OP_SWAP, _swap);
  ASSIGN(OP_ELEM, _elem);

  ASSIGN(OP_LD_GLO, _ld_glo);
  ASSIGN(OP_LD_LOC, _ld_loc);
  ASSIGN(OP_LD_ARG, _ld_arg);
  ASSIGN(OP_LD_CLO, _ld_clo);

  ASSIGN(OP_ST_GLO, _st_glo);
  ASSIGN(OP_ST_LOC, _st_loc);
  ASSIGN(OP_ST_ARG, _st_arg);
  ASSIGN(OP_ST_CLO, _st_clo);

  ASSIGN(OP_CJMP, _cjmp);
  ASSIGN(OP_BEGIN, _begin);
  ASSIGN(OP_BEGIN_CLOSURE, _begin_closure);
  ASSIGN(OP_CLOSURE, _closure);
  ASSIGN(OP_CALLC, _callc);
  ASSIGN(OP_CALL, _call);
  ASSIGN(OP_TAG, _tag);
  ASSIGN(OP_ARRAY, _array);
  ASSIGN(OP_FAIL, _fail);
  ASSIGN(OP_LINE, _line);
  ASSIGN(OP_PATT, _patt);
  ASSIGN(OP_BARRAY, _barray);
  ASSIGN(OP_EOF, _eof);

  NEXT;

_add:
  {
    BINOP(+);
  }

_sub:
  {
    BINOP(-);
  }

_mul:
  {
    BINOP(*);
  }

_div:
  {
    BINOP(/);
  }

_mod:
  {
    BINOP(%);
  }

_and:
  {
    BINOP(&&);
  }

_or:
  {
    BINOP(||);
  }

_lt:
  {
    BINOP(<);
  }

_le:
  {
    BINOP(<=);
  }

_gt:
  {
    BINOP(>);
  }

_ge:
  {
    BINOP(>=);
  }

_eq:
  {
    BINOP(==);
  }

_neq:
  {
    BINOP(!=);
  }

_const:
  stack.push(BOX(readI32()));
  NEXT;

_string:
  {
    auto rawString = fromPool();
    auto ptr = Bstring(reinterpret_cast<aint*>(&rawString));
    stack.push(reinterpret_cast<ptrt>(ptr));
    NEXT;
  }

_sexp:
  {
    auto tag = reinterpret_cast<const char*>(fromPool());
    stack.push(LtagHash(tag));

    auto n = readI32();
    auto ptr = Bsexp(reinterpret_cast<aint*>(stack.getAddr(n + 1)), BOX(n + 1));
    stack.push(reinterpret_cast<ptrt>(ptr));
    stack.popN(n + 1);
    NEXT;
  }

_sta:
  {
    auto value = stack.pop();
    auto index = stack.pop();
    auto aggregate = stack.pop();
    auto updated = Bsta(reinterpret_cast<void*>(aggregate), index,
                        reinterpret_cast<void*>(value));
    stack.push(reinterpret_cast<ptrt>(updated));
    NEXT;
  }

_jmp:  // TODO
_end:  // TODO

_drop:
  stack.pop();
  NEXT;

_dup:
  stack.push(stack.top());
  NEXT;

_swap:
  stack.swap();
  NEXT;

_elem:
  {
    auto index = stack.pop();
    auto aggregate = stack.pop();
    auto value = Belem(reinterpret_cast<void*>(aggregate), index);
    stack.push(reinterpret_cast<ptrt>(value));
    NEXT;
  }

_ld_glo:         // TODO
_ld_loc:         // TODO
_ld_arg:         // TODO
_ld_clo:         // TODO
_st_glo:         // TODO
_st_loc:         // TODO
_st_arg:         // TODO
_st_clo:         // TODO
_cjmp:           // TODO
_begin:          // TODO
_begin_closure:  // TODO
_closure:        // TODO
_callc:          // TODO
_call:           // TODO
_tag:            // TODO

_array:  // TODO

_fail:  // TODO
_line:  // TODO
_patt:  // TODO

_barray:
  {
    auto n = readI32();
    auto elems = stack.getAddr(n);
    auto array = Barray(reinterpret_cast<aint*>(elems), BOX(n));
    stack.popN(n);
    stack.push(reinterpret_cast<ptrt>(array));
    NEXT;
  }

_eof:
  return UNBOX(stack.pop());
}
