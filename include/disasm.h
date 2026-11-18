#pragma once

#include <cstdint>

#include "byte-reader.h"
#include "tokens.h"

struct DecodedInsn {
  Token op;
  std::size_t length;
  std::int32_t target = -1;
};

inline DecodedInsn decodeInsn(ByteReader& reader) {
  const std::size_t start = reader.pos();
  const auto op = static_cast<Token>(reader.u8());
  std::int32_t target = -1;

  switch (op) {
    case OP_CONST:
    case OP_STRING:
    case OP_LD_GLO:
    case OP_LD_LOC:
    case OP_LD_ARG:
    case OP_LD_CLO:
    case OP_ST_GLO:
    case OP_ST_LOC:
    case OP_ST_ARG:
    case OP_ST_CLO:
    case OP_CALLC:
    case OP_ARRAY:
    case OP_LINE:
    case OP_BARRAY:
      reader.skip(4);
      break;

    case OP_JMP:
    case OP_CJMP_Z:
    case OP_CJMP_NZ:
      target = reader.i32();
      break;

    case OP_SEXP:
    case OP_BEGIN:
    case OP_TAG:
    case OP_FAIL:
    case OP_FAIL_KEEP:
      reader.skip(8);
      break;

    case OP_CALL:
      target = reader.i32();
      reader.skip(4);
      break;

    case OP_BEGIN_CLOSURE:
      reader.skip(12);
      break;

    case OP_CLOSURE: {
      target = reader.i32();
      const std::int32_t n = reader.i32();
      reader.skip(static_cast<std::size_t>(n) * 5);  // n * (1B kind + 4B index)
      break;
    }

    default:
      break;  // 0 immediate bytes
  }

  return {op, reader.pos() - start, target};
}
