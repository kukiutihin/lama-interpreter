#include "virtual-machine.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "app-runtime.h"
#include "runtime_common.h"
#include "stack.h"
#include "tokens.h"

Stack VirtualMachine::allocateStack() {
  Stack stack;

  // reserves space for globals in stack
  for (std::size_t i = 0; i < globalsCount; i++) stack.push(BOX(0));

  // nobody saves ip for main function (call is not called)
  // we will save it here manually so that main can return control to EOF
  stack.push(BOX(0)); // sp
  stack.push(BOX(reinterpret_cast<ptrt>(eof))); // ip

  return stack;
}

#define ASSIGN(op, label) handlers[(op)] = &&label;
#define NEXT goto* handlers[*ip++];

#define BINOP(fn)                                  \
  {                                                \
    auto b = reinterpret_cast<void*>(stack.pop()); \
    auto a = reinterpret_cast<void*>(stack.pop()); \
    stack.push(fn(a, b));                          \
    NEXT;                                          \
  }

void VirtualMachine::interpret(uint8_t* ipBase) {
  static void* handlers[256];

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

  ASSIGN(OP_CJMP_Z, _cjmp_z);
  ASSIGN(OP_CJMP_NZ, _cjmp_nz);

  ASSIGN(OP_BEGIN, _begin);
  ASSIGN(OP_BEGIN_CLOSURE, _begin_closure);
  ASSIGN(OP_CLOSURE, _closure);
  ASSIGN(OP_CALLC, _callc);
  ASSIGN(OP_CALL, _call);
  ASSIGN(OP_TAG, _tag);
  ASSIGN(OP_ARRAY, _array);

  ASSIGN(OP_FAIL, _fail);
  ASSIGN(OP_FAIL_KEEP, _fail_keep);

  ASSIGN(OP_LINE, _line);

  ASSIGN(OP_PATT_STR_CMP, _patt_str_cmp);
  ASSIGN(OP_PATT_STRING, _patt_string);
  ASSIGN(OP_PATT_ARRAY, _patt_array);
  ASSIGN(OP_PATT_SEXP, _patt_sexp);
  ASSIGN(OP_PATT_BOXED, _patt_boxed);
  ASSIGN(OP_PATT_UNBOXED, _patt_unboxed);
  ASSIGN(OP_PATT_CLOSURE, _patt_closure);

  ASSIGN(OP_BARRAY, _barray);
  ASSIGN(OP_EOF, _eof);

  auto stack = allocateStack();
  std::uint8_t* pool = constantPool;
  std::size_t fp = globalsCount;
  auto ip = ipBase;

  auto read = [&ip]<typename T> [[gnu::always_inline]] (std::size_t size) -> T {
    T var;
    std::memcpy(&var, ip, size);
    ip += size;
    return var;
  };

  auto readI32 = [&] [[gnu::always_inline]] () -> std::int32_t {
    return read.operator()<std::int32_t>(4);
  };

  auto readU8 = [&] [[gnu::always_inline]] () -> std::uint8_t {
    return read.operator()<std::uint8_t>(1);
  };

  NEXT;

_add:
  BINOP(Ls__Infix_43);

_sub:
  BINOP(Ls__Infix_45);

_mul:
  BINOP(Ls__Infix_42);

_div:
  BINOP(Ls__Infix_47);

_mod:
  BINOP(Ls__Infix_37);

_and:
  BINOP(Ls__Infix_3838);

_or:
  BINOP(Ls__Infix_3333);

_lt:
  BINOP(Ls__Infix_60);

_le:
  BINOP(Ls__Infix_6061);

_gt:
  BINOP(Ls__Infix_62);

_ge:
  BINOP(Ls__Infix_6261);

_eq:
  BINOP(Ls__Infix_6161);

_neq:
  BINOP(Ls__Infix_3361);

_const:
  stack.push(BOX(readI32()));
  NEXT;

_string:
  {
    auto rawString = &pool[readI32()];
    auto ptr = Bstring(reinterpret_cast<aint*>(&rawString));
    stack.push(reinterpret_cast<ptrt>(ptr));
    NEXT;
  }

_sexp:
  {
    auto tag = reinterpret_cast<const char*>(&pool[readI32()]);
    stack.push(LtagHash(tag));

    auto n = readI32();
    auto ptr =
        Bsexp(reinterpret_cast<aint*>(stack.byOffset(stack.offset() - (n + 1))),
              BOX(n + 1));

    stack.popN(n + 1);
    stack.push(reinterpret_cast<ptrt>(ptr));
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

_ld_glo:
  {
    auto index = readI32();
    stack.push(*stack.byOffset(index));
    NEXT;
  }

_ld_loc:
  {
    auto index = readI32();
    stack.push(*stack.byOffset(fp + index));
    NEXT;
  }

_ld_arg:
  {
    auto index = readI32();
    auto savedSP = UNBOX(*stack.byOffset(fp - 3));
    stack.push(*(stack.byOffset(savedSP + index)));
    NEXT;
  }

_ld_clo:
  {
    auto offset = readI32();
    auto closure = *stack.byOffset(fp - 4);

    stack.push(reinterpret_cast<ptrt>(
        Belem(reinterpret_cast<void*>(closure), BOX(offset + 1))));

    NEXT;
  }

_st_glo:
  {
    auto index = readI32();
    *stack.byOffset(index) = stack.top();
    NEXT;
  }

_st_loc:
  {
    auto index = readI32();
    *stack.byOffset(fp + index) = stack.top();
    NEXT;
  }

_st_arg:
  {
    auto index = readI32();
    auto savedSP = UNBOX(*stack.byOffset(fp - 3));
    *(stack.byOffset(savedSP + index)) = stack.top();
    NEXT;
  }

_st_clo:
  {
    auto offset = readI32();
    auto closure = *stack.byOffset(fp - 4);

    stack.push(reinterpret_cast<ptrt>(
        Bsta(reinterpret_cast<void*>(closure), BOX(offset + 1),
             reinterpret_cast<void*>(stack.pop()))));
    NEXT;
  }

_jmp:
  ip = ipBase + readI32();
  NEXT;

_cjmp_z:
  {
    auto offset = readI32();
    if (UNBOX(stack.pop()) == 0) ip = ipBase + offset;
    NEXT;
  }

_cjmp_nz:
  {
    auto offset = readI32();
    if (UNBOX(stack.pop()) != 0) ip = ipBase + offset;
    NEXT;
  }

_call:
  {
    auto offset = readI32();
    auto argsCount = readI32();

    // positive offset points to BEGIN in bytecode
    if (offset >= 0) {
      stack.push(BOX(stack.offset() - argsCount));  // saving sp
      stack.push(BOX(reinterpret_cast<ptrt>(ip)));  // saving ip
      ip = ipBase + offset;

      // negative points to external (built-in) function
    } else {
      auto name =
          std::string_view(reinterpret_cast<const char*>(&pool[(-offset - 1)]));

      if (name == "write")
        stack.push(Lwrite(stack.pop()));

      else if (name == "read")
        stack.push(Lread());
    }

    NEXT;
  }

_begin:
  {
    readI32();  // args
    auto localsCount = readI32();

    stack.push(BOX(fp));  // saving fp
    fp = stack.offset();

    // reserving space in stack for locals
    for (std::size_t i = 0; i < localsCount; i++) stack.push(BOX(0));
    NEXT;
  }

_end:
  {
    /*
     * -----------
     *    arg_1     <- saved sp
     * -----------
     *     ...
     * -----------
     *    arg_n
     * -----------
     *  saved sp     saved in call
     * -----------
     *  saved ip     saved in call
     * -----------
     *  saved fp     saved in begin
     * -----------
     *   local_1     <- new fp
     * -----------
     *     ...
     * -----------
     *   local_n
     * -----------
     * return value
     * -----------
     *               <- sp
     */
    auto returnValue = stack.pop();
    stack.jumpTo(fp);
    fp = UNBOX(stack.pop());
    ip = reinterpret_cast<std::uint8_t*>(UNBOX(stack.pop()));
    stack.jumpTo(UNBOX(stack.pop()));  // jumps to saved sp
    stack.push(returnValue);
    NEXT;
  }

_begin_closure:
  {
    readI32();  // args
    auto localsCount = readI32();
    readI32();  // captures

    stack.push(BOX(fp));  // saving fp

    fp = stack.offset();

    // reserving space in stack for locals
    for (std::size_t i = 0; i < localsCount; i++) stack.push(BOX(0));
    NEXT;
  }

_closure:
  {
    auto ipOffset = readI32();
    auto capturesCount = readI32();
    stack.push(BOX(ipOffset));

    for (std::size_t i = 0; i < capturesCount; i++) {
      auto kind = readU8();
      auto offset = readI32();

      switch (kind) {
        case 0:  // global
          stack.push(*stack.byOffset(offset));
          break;

        case 1:  // local
          stack.push(*stack.byOffset(fp + offset));
          break;

        case 2: {  // arg
          auto savedSP = UNBOX(*stack.byOffset(fp - 3));
          stack.push(*(stack.byOffset(savedSP + offset)));
          break;
        }

        case 3: {  // capture
          auto savedSP = UNBOX(*stack.byOffset(fp - 3));
          auto closure = *(stack.byOffset(savedSP + offset));
          stack.push(reinterpret_cast<ptrt>(
              Belem(reinterpret_cast<void*>(closure), BOX(offset + 1))));
          break;
        }
      }
    }

    auto closure = reinterpret_cast<ptrt>(
        Bclosure(reinterpret_cast<aint*>(
                     stack.byOffset(stack.offset() - capturesCount - 1)),
                 BOX(capturesCount + 1)));

    stack.popN(capturesCount + 1);
    stack.push(closure);
    NEXT;
  }

_callc:
  {
    auto argsCount = readI32();
    auto offset = UNBOX(reinterpret_cast<ptrt>(
        Belem(reinterpret_cast<void*>(stack.top()), BOX(0))));  // closure[0]

    stack.push(BOX(stack.offset() - argsCount - 1));  // saving sp
    stack.push(BOX(reinterpret_cast<ptrt>(ip)));      // saving ip
    ip = ipBase + offset;
    NEXT;
  }

_tag:
  {
    auto name = LtagHash(reinterpret_cast<const char*>(&pool[readI32()]));
    auto fieldsCount = readI32();
    stack.push(
        Btag(reinterpret_cast<void*>(stack.pop()), name, BOX(fieldsCount)));
    NEXT;
  }

_array:
  {
    auto elementsCount = readI32();
    stack.push(
        Barray_patt(reinterpret_cast<void*>(stack.pop()), BOX(elementsCount)));
    NEXT;
  }

_fail:
  {
    auto line = readI32();
    auto col = readI32();
    stack.pop();
    failure("Match error at %d:%d\n", line, col);
    NEXT;
  }

_fail_keep:
  {
    auto line = readI32();
    auto col = readI32();
    failure("Match error at %d:%d\n", line, col);
    NEXT;
  }

_line:
  readI32();
  NEXT;

_patt_str_cmp:
  stack.push(Bstring_patt(reinterpret_cast<void*>(stack.pop()),
                          reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_string:
  stack.push(Bstring_tag_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_array:
  stack.push(Barray_tag_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_sexp:
  stack.push(Bsexp_tag_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_boxed:
  stack.push(Bboxed_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_unboxed:
  stack.push(Bunboxed_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_patt_closure:
  stack.push(Bclosure_tag_patt(reinterpret_cast<void*>(stack.pop())));
  NEXT;

_barray:
  {
    auto n = readI32();
    auto elems = stack.byOffset(stack.offset() - n);
    auto array = Barray(reinterpret_cast<aint*>(elems), BOX(n));
    stack.popN(n);
    stack.push(reinterpret_cast<ptrt>(array));
    NEXT;
  }

_eof:
}
