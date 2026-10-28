#include "virtual-machine.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "runtime.hpp"
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
  static void* handlers[16];
  Stack stack(&__gc_stack_top, &__gc_stack_bottom);

  ASSIGN(OP_BINOP_ADD, _add);
  ASSIGN(OP_BINOP_SUB, _sub);
  ASSIGN(OP_BINOP_MUL, _mul);
  ASSIGN(OP_BINOP_DIV, _div);
  ASSIGN(OP_BINOP_MOD, _mod);
  ASSIGN(OP_BINOP_AND, _and);
  ASSIGN(OP_BINOP_OR, _or);
  ASSIGN(OP_BINOP_LT, _lt);
  ASSIGN(OP_BINOP_LE, _le);
  ASSIGN(OP_BINOP_GT, _gt);
  ASSIGN(OP_BINOP_GE, _ge);
  ASSIGN(OP_BINOP_EQ, _eq);
  ASSIGN(OP_BINOP_NEQ, _neq);
  ASSIGN(OP_CONST, _const);
  ASSIGN(OP_STRING, _string);
  ASSIGN(OP_END, _end);

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
    BINOP(or);
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
  {
    aint value;
    std::memcpy(&value, ip, CONST_SIZE);
    ip += CONST_SIZE;
    stack.push(BOX(value));
    NEXT;
  }

_string:
  {
    std::uint32_t index;
    std::memcpy(&index, ip, CONST_SIZE);
    ip += CONST_SIZE;
    const char* rawString = constantPool[index];
    stack.push(
        reinterpret_cast<ptrt>(Bstring(reinterpret_cast<aint*>(&rawString))));
    NEXT;
  }

_end:
  return stack.pop();
}
