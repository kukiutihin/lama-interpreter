#pragma once

#include <cstdint>

enum DESIGNATION : uint8_t { GLO = 0, LOC, ARG, CLO };

// every opcode size = [1B];
// (..., x, y -> ..., a):  takes y, x from operand stack, stores a;
// [4B const]: immediate operand (const of size 4B) stored right after opcode
#define TOKEN_LIST                                                             \
                                                                               \
  /* binary operations */                                                      \
  /* (..., x, y -> ..., x <op> y) */                                           \
  X(OP_BINOP_ADD, 0x01)                                                        \
  X(OP_BINOP_SUB, 0x02)                                                        \
  X(OP_BINOP_MUL, 0x03)                                                        \
  X(OP_BINOP_DIV, 0x04)                                                        \
  X(OP_BINOP_MOD, 0x05)                                                        \
  X(OP_BINOP_LT, 0x06)                                                         \
  X(OP_BINOP_LE, 0x07)                                                         \
  X(OP_BINOP_GT, 0x08)                                                         \
  X(OP_BINOP_GE, 0x09)                                                         \
  X(OP_BINOP_EQ, 0x0A)                                                         \
  X(OP_BINOP_NEQ, 0x0B)                                                        \
  X(OP_BINOP_AND, 0x0C)                                                        \
  X(OP_BINOP_OR, 0x0D)                                                         \
                                                                               \
  /* places const on stack */                                                  \
  /* (... -> ..., const) */                                                    \
  /* [4B const] */                                                             \
  X(OP_CONST, 0x10)                                                            \
                                                                               \
  /* places string from costant pool on stack */                               \
  /* (... -> ..., ptr to string) */                                            \
  /* [4B constant pool offset] */                                              \
  X(OP_STRING, 0x11)                                                           \
                                                                               \
  /* takes immediate tag and n expressions from stack, */                      \
  /* creates s-expression and places ptr to it on stack */                     \
  /* */                                                                        \
  /* (..., expr_1, ..., expr_n  -> ..., ptr to sexp) */                        \
  /* [4B constant pool offset][n: 4B exprs count] */                           \
  X(OP_SEXP, 0x12)                                                             \
                                                                               \
  /* places value to an aggregate (array, string, sexp) at given index */      \
  /* (..., aggregate, index, value  -> ..., value) */                          \
  X(OP_STA, 0x14)                                                              \
                                                                               \
  /* unconditional jump to given byte offset */                                \
  /* (... -> ...) */                                                           \
  /* [4B offset] */                                                            \
  X(OP_JMP, 0x15)                                                              \
                                                                               \
  X(OP_END, 0x16)                                                              \
                                                                               \
  /* (..., el  -> ...) */                                                      \
  X(OP_DROP, 0x18)                                                             \
                                                                               \
  /* (..., el  -> ..., el, el) */                                              \
  X(OP_DUP, 0x19)                                                              \
                                                                               \
  /* (..., el1, el2  -> ..., el2, el1) */                                      \
  X(OP_SWAP, 0x1A)                                                             \
                                                                               \
  /* loads value from aggregate */                                             \
  /* (..., aggregate, index -> ..., value) */                                  \
  X(OP_ELEM, 0x1B)                                                             \
                                                                               \
  /* GLO - global variable */                                                  \
  /* LOC - local variable */                                                   \
  /* ARG - function argument */                                                \
  /* CLO - closure capture */                                                  \
                                                                               \
  /* loads value from ... to stack */                                          \
  /* (... -> ..., value) */                                                    \
  /* [4B index] */                                                             \
  X(OP_LD_GLO, 0x20)                                                           \
  X(OP_LD_LOC, 0x21)                                                           \
  X(OP_LD_ARG, 0x22)                                                           \
  X(OP_LD_CLO, 0x23)                                                           \
                                                                               \
  /* stores value from stack top to ... */                                     \
  /* (..., value -> ..., value) */                                             \
  /* [4B index] */                                                             \
  X(OP_ST_GLO, 0x40)                                                           \
  X(OP_ST_LOC, 0x41)                                                           \
  X(OP_ST_ARG, 0x42)                                                           \
  X(OP_ST_CLO, 0x43)                                                           \
                                                                               \
  /* conditional jumps to given byte offset */                                 \
  /* (..., value -> ...) */                                                    \
  /* [4B offset] */                                                            \
  X(OP_CJMP_Z, 0x50)  /* if value == 0 */                                      \
  X(OP_CJMP_NZ, 0x51) /* if value != 0 */                                      \
                                                                               \
  /* OP_BEGIN[_CLOSURE] is function prologue section */                        \
  /* saves fp onto the stack */                                                \
  /* reserves slots for locals on stack */                                     \
  /* */                                                                        \
  /* (..., arg_1, .., arg_n, sp, ip -> */                                      \
  /* ..., arg_1, .., arg_n, sp, ip, fp, local_1, .., local_m) */               \
  /* */                                                                        \
  /* [4B args count][4B locals count] */                                       \
  X(OP_BEGIN, 0x52)                                                            \
                                                                               \
  /* (..., arg_1, .., arg_n, closure, sp, ip -> */                             \
  /* ..., arg_1, .., arg_n, closure, sp, ip, fp, local_1, .., local_m) */      \
  /* */                                                                        \
  /* [4B args count][4B locals count][4B captures count] */                    \
  X(OP_BEGIN_CLOSURE, 0x53)                                                    \
                                                                               \
  /* (... -> ..., closure) */                                                  \
  /* [4B byte offset][4B captures count]([1B capture kind][4B capture value])* \
   */                                                                          \
  /* 0 -> global, 1 -> local, 2 -> arg, 3 -> capture */                        \
  X(OP_CLOSURE, 0x54)                                                          \
                                                                               \
  /* same as call for internal, but jumps to OP_BEGIN_CLOSURE */               \
  /* offset is closure[0] */                                                   \
  /* (..., arg_1, .., arg_n, closure -> ..., arg_1, .., arg_n, closure, sp,    \
   * ip) */                                                                    \
  /* [4B args count] */                                                        \
  X(OP_CALLC, 0x55)                                                            \
                                                                               \
  /* saves sp and ip, jumps to BEGIN on given offset */                        \
  /* negative offset means external function */                                \
  /* */                                                                        \
  /* FOR INTERNAL: */                                                          \
  /* (..., arg_1, .., arg_n -> ..., arg_1, .., arg_n, sp, ip) */               \
  /* [4B offset][4B args count] */                                             \
  X(OP_CALL, 0x56)                                                             \
                                                                               \
  /* checks that top stack value is SEXP of given arity */                     \
  /* (..., value -> ..., result) */                                            \
  /* [4B elements count] */                                                    \
  X(OP_TAG, 0x57)                                                              \
                                                                               \
  /* checks that top stack value is ARRAY of given length */                   \
  /* (..., value -> ..., result) */                                            \
  /* [4B elements count] */                                                    \
  X(OP_ARRAY, 0x58)                                                            \
                                                                               \
  /* raise match error (terminate program) */                                  \
  /* (..., top -> ...) */                                                      \
  /* [4B line index][4B column index] */                                       \
  X(OP_FAIL, 0x59)                                                             \
                                                                               \
  /* saves stack top */                                                        \
  /* (..., top -> .... top) */                                                 \
  X(OP_FAIL_KEEP, 0x5A)                                                        \
                                                                               \
  X(OP_LINE, 0x5B)                                                             \
                                                                               \
  /* checks that two top values on stack are strings and they are equal */     \
  /* (..., strA, strB -> ..., result) */                                       \
  X(OP_PATT_STR_CMP, 0x60)                                                     \
                                                                               \
  /* OP_PATT_* checks that the value is * */                                   \
  /* (..., value -> ..., result) */                                            \
  X(OP_PATT_STRING, 0x61)                                                      \
  X(OP_PATT_ARRAY, 0x62)                                                       \
  X(OP_PATT_SEXP, 0x63)                                                        \
  X(OP_PATT_BOXED, 0x64)                                                       \
  X(OP_PATT_UNBOXED, 0x65)                                                     \
  X(OP_PATT_CLOSURE, 0x66)                                                     \
                                                                               \
  /* takes n elements from stack and store pointer to an array */              \
  /* (..., el_1, ..., el_n  -> ..., ptr to array) */                           \
  /* [4B elements count] */                                                    \
  X(OP_BARRAY, 0x74)                                                           \
                                                                               \
  X(OP_EOF, 0xFF)

#define X(name, val) name = val,
enum Token : uint8_t { TOKEN_LIST };
#undef X

inline const char* opToString(enum Token op) {
  switch (op) {
#define X(name, val) \
  case name:         \
    return #name;
    TOKEN_LIST
#undef X
  }
}
