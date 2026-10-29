#pragma once

#include <cstdint>

// every opcode size = [1B];
// (..., x, y -> ..., a):  takes y, x from operand stack, stores a;
// [4B const]: immediate operand (const of size 4B) stored right after opcode
enum Token : uint8_t {

  // binary operations
  // (..., x, y -> ..., x <op> y)
  OP_BINOP_ADD = 1,
  OP_BINOP_SUB,
  OP_BINOP_MUL,
  OP_BINOP_DIV,
  OP_BINOP_MOD,
  OP_BINOP_LT,
  OP_BINOP_LE,
  OP_BINOP_GT,
  OP_BINOP_GE,
  OP_BINOP_EQ,
  OP_BINOP_NEQ,
  OP_BINOP_AND,
  OP_BINOP_OR,

  // places const on stack
  // (... -> ..., const)
  // [4B const]
  OP_CONST,

  // places string from costant pool on stack
  // (... -> ..., ptr to string)
  // [4B constant pool offset]
  OP_STRING,

  // takes immediate tag and n expressions from stack,
  // creates s-expression and places ptr to it on stack
  //
  // (..., expr_1, ..., expr_n  -> ..., ptr to sexp)
  // [4B constant pool offset][n: 4B exprs count]
  OP_SEXP,

  // places value to an aggregate (array, string, sexp) at given index
  // (..., aggregate, index, value  -> ..., value)
  OP_STA,

  OP_JMP,
  OP_END,

  // (..., el  -> ...)
  OP_DROP,

  // (..., el  -> ..., el, el)
  OP_DUP,

  // (..., el1, el2  -> ..., el2, el1)
  OP_SWAP,

  // loads value from aggregate
  // (..., aggregate, index -> ..., value)
  OP_ELEM,

  OP_LD_GLO,
  OP_LD_LOC,
  OP_LD_ARG,
  OP_LD_CLO,

  OP_ST_GLO,
  OP_ST_LOC,
  OP_ST_ARG,
  OP_ST_CLO,

  OP_CJMP,
  OP_BEGIN,
  OP_BEGIN_CLOSURE,
  OP_CLOSURE,
  OP_CALLC,
  OP_CALL,
  OP_TAG,
  OP_ARRAY,
  OP_FAIL,
  OP_LINE,
  OP_PATT,

  // takes n elements from stack and store pointer to an array
  // (..., el_1, ..., el_n  -> ..., ptr to array)
  // [4B elements count]
  OP_BARRAY,

  OP_EOF,

  // OP_LDA,  // Designation
  // OP_STI,
  // OP_LABEL,   // string
  // OP_FLABEL,  // string
  // OP_SLABEL,  // string
  // OP_PROTO,    // string, string
  // OP_PPROTO,   // string, string
  // OP_PCALLC,   // int, bool
  // OP_RET,
  // OP_EXTERN,  // string, bool
  // OP_PUBLIC,  // string, bool
  // OP_IMPORT,  // string
};
