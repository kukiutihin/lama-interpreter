#include "virtual-machine.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "runtime.hpp"
#include "tokens.hpp"

#define POP stack[--current]
#define PUSH(value) stack[current++] = value;

#define ASSIGN(op, label) handlers[(op)] = &&label;
#define NEXT goto* handlers[*ip++];

static constexpr std::size_t STACK_ELEM_SIZE = sizeof(ptrt);
static constexpr std::size_t CONST_SIZE = sizeof(int32_t);

#define BINOP(op)      \
  aint b = UNBOX(POP); \
  aint a = UNBOX(POP); \
  PUSH(BOX(a op b));   \
  NEXT;

ptrt VirtualMachine::interpret(uint8_t* ip) {
  static void* handlers[16];
  ptrt* stack = new ptrt[100];
  std::size_t current = 0;

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

_add: { BINOP(+); }
_sub: { BINOP(-); }
_mul: { BINOP(*); }
_div: { BINOP(/); }
_mod: { BINOP(%); }
_and: { BINOP(&&); }
_or: { BINOP(or); }
_lt: { BINOP(<); }
_le: { BINOP(<=); }
_gt: { BINOP(>); }
_ge: { BINOP(>=); }
_eq: { BINOP(==); }
_neq: { BINOP(!=); }

_const: {
  aint value;
  std::memcpy(&value, ip, CONST_SIZE);
  ip += CONST_SIZE;
  PUSH(BOX(value));
  NEXT;
}

_string: {
  std::uint32_t index;
  std::memcpy(&index, ip, CONST_SIZE);
  ip += CONST_SIZE;
  const char* rawString = constantPool[index];
  PUSH(reinterpret_cast<ptrt>(Bstring(reinterpret_cast<aint*>(&rawString))));
  NEXT;
}

_end:
  return POP;
}