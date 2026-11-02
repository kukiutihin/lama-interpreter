#pragma once

#include <cstdint>

// every opcode size = [1B];
// (..., x, y -> ..., a):  takes y, x from operand stack, stores a;
// [4B const]: immediate operand (const of size 4B) stored right after opcode
enum Token : uint8_t {

  // binary operations
  // (..., x, y -> ..., x <op> y)
  OP_BINOP_ADD = 0x01,
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
  OP_CONST = 0x10,

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
  OP_STA = 0x14,

  // unconditional jump to given byte offset
  // (... -> ...)
  // [4B offset]
  OP_JMP,

  OP_END,

  // (..., el  -> ...)
  OP_DROP = 0x18,

  // (..., el  -> ..., el, el)
  OP_DUP,

  // (..., el1, el2  -> ..., el2, el1)
  OP_SWAP,

  // loads value from aggregate
  // (..., aggregate, index -> ..., value)
  OP_ELEM,

  // GLO - global variable
  // LOC - local variable
  // ARG - function argument
  // CLO - closure capture

  // loads value from ... to stack
  // (... -> ..., value)
  // [4B index]
  OP_LD_GLO = 0x20,
  OP_LD_LOC,
  OP_LD_ARG,
  OP_LD_CLO,

  // stores value from stack top to ...
  // (..., value -> ..., value)
  // [4B index]
  OP_ST_GLO = 0x40,
  OP_ST_LOC,
  OP_ST_ARG,
  OP_ST_CLO,

  // conditional jumps to given byte offset
  // (..., value -> ...)
  // [4B offset]
  OP_CJMP_Z = 0x50,  // if value == 0
  OP_CJMP_NZ,        // if value != 0

  // OP_BEGIN[_CLOSURE] is function prologue section
  // saves fp onto the stack
  // reserves slots for locals on stack
  //
  // (..., arg_1, .., arg_n, sp, ip ->
  // ..., arg_1, .., arg_n, sp, ip, fp, local_1, .., local_m)
  //
  // [4B args count][4B locals count]
  OP_BEGIN = 0x52,

  // (..., arg_1, .., arg_n, closure, sp, ip ->
  // ..., arg_1, .., arg_n, closure, sp, ip, fp, local_1, .., local_m)
  //
  // [4B args count][4B locals count][4B captures count]
  OP_BEGIN_CLOSURE,

  // (... -> ..., closure)
  // [4B byte offset][4B captures count]([1B capture kind][4B capture value])*
  // 0 -> global, 1 -> local, 2 -> arg, 3 -> capture
  OP_CLOSURE,

  // same as call for internal, but jumps to OP_BEGIN_CLOSURE
  // offset is closure[0]
  // (..., arg_1, .., arg_n, closure -> ..., arg_1, .., arg_n, closure, sp, ip)
  // [4B args count]
  OP_CALLC,

  // saves sp and ip, jumps to BEGIN on given offset
  // negative offset means external function
  //
  // FOR INTERNAL:
  // (..., arg_1, .., arg_n -> ..., arg_1, .., arg_n, sp, ip)
  // [4B offset][4B args count]
  OP_CALL,

  // checks that top stack value is SEXP of given arity
  // (..., value -> ..., result)
  // [4B elements count]
  OP_TAG,

  // checks that top stack value is ARRAY of given length
  // (..., value -> ..., result)
  // [4B elements count]
  OP_ARRAY,

  // raise match error (terminate program)
  // (..., top -> ...)
  // [4B line index][4B column index]
  OP_FAIL,

  // saves stack top
  // (..., top -> .... top)
  OP_FAIL_KEEP,

  OP_LINE = 0x5B,

  // checks that two top values on stack are strings and they are equal
  // (..., strA, strB -> ..., result)
  OP_PATT_STR_CMP = 0x60,

  // OP_PATT_* checks that the value is *
  // (..., value -> ..., result)
  OP_PATT_STRING,
  OP_PATT_ARRAY,
  OP_PATT_SEXP,
  OP_PATT_BOXED,
  OP_PATT_UNBOXED,
  OP_PATT_CLOSURE,

  // takes n elements from stack and store pointer to an array
  // (..., el_1, ..., el_n  -> ..., ptr to array)
  // [4B elements count]
  OP_BARRAY = 0x74,

  OP_EOF = 0xFF,
};
