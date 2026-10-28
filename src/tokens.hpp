#pragma once
#include <cstdint>


// enum Patt : uint8_t { StrCmp, String, Array, SExp, Boxed, UnBoxed, Closure };

enum Token : uint8_t {
  // [1B]
  OP_BINOP_ADD = 0,
  OP_BINOP_SUB,
  OP_BINOP_MUL,
  OP_BINOP_DIV,
  OP_BINOP_MOD,
  OP_BINOP_AND,
  OP_BINOP_OR,
  OP_BINOP_LT,
  OP_BINOP_LE,
  OP_BINOP_GT,
  OP_BINOP_GE,
  OP_BINOP_EQ,
  OP_BINOP_NEQ,
  OP_END,

  OP_CONST,   // [1B CONST][4B int]
  OP_STRING,  // [1B STRING][4B const pool index]

  // OP_SEXP,    // string, int
  // OP_DROP,
  // OP_DUP,
  // OP_SWAP,
  // OP_LD,   // Designation
  // OP_LDA,  // Designation
  // OP_ST,   // Designation
  // OP_STI,
  // OP_STA,
  // OP_ELEM,
  // OP_LABEL,   // string
  // OP_FLABEL,  // string
  // OP_SLABEL,  // string
  // OP_JMP,     // string
  // OP_CJMP,    // string, string
  // OP_BEGIN,   // string, int, int, [Designation], [string], [Scope]
  // OP_END,
  // OP_CLOSURE,  // string, [Designation]
  // OP_PROTO,    // string, string
  // OP_PPROTO,   // string, string
  // OP_PCALLC,   // int, bool
  // OP_CALLC,    // int, bool
  // OP_CALL,     // string, int, bool
  // OP_RET,
  // OP_TAG,     // string, int
  // OP_ARRAY,   // int
  // OP_PATT,    // Patt
  // OP_FAIL,    // Loc, bool
  // OP_EXTERN,  // string, bool
  // OP_PUBLIC,  // string, bool
  // OP_IMPORT,  // string
  // OP_LINE     // int
};