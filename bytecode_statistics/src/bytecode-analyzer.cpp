#include "bytecode-analyzer.h"

#include <queue>

#include "bool-pair-vec.h"
#include "byte-reader.h"
#include "disasm.h"
#include "tokens.h"

BoolPairVec BytecodeAnalyzer::reachability() {
  ByteReader reader(bytecode.code, bytecode.codeSize);

  // first - visited
  // second - isSeparator (cannot be used to count pairs)
  BoolPairVec v(bytecode.codeSize);

  std::queue<std::size_t> entries;
  entries.push(0);

  while (!entries.empty()) {
    reader.jumpTo(entries.front());
    entries.pop();

    if (v.isFirst(reader.pos())) continue;

    while (true) {
      v.setFirst(reader.pos());
      auto decoded = decodeInsn(reader);

      switch (decoded.op) {
        case OP_JMP:
          v.setSecond(reader.pos());
          v.setSecond(decoded.target);
          if (!v.isFirst(decoded.target)) entries.push(decoded.target);
          goto end;

        case OP_CJMP_NZ:
        case OP_CJMP_Z:
          v.setSecond(reader.pos());
          v.setSecond(decoded.target);
          if (!v.isFirst(decoded.target)) entries.push(decoded.target);
          break;

        case OP_CALL:
          if (decoded.target >= 0) {
            v.setSecond(reader.pos());
            v.setSecond(decoded.target);
          }

        case OP_CALLC:
          if (decoded.target >= 0 && !v.isFirst(decoded.target))
            entries.push(decoded.target);
          break;

        case OP_EOF:
        case OP_FAIL:
        case OP_END:
          goto end;

        default:
          break;
      }
    }
  end:
  }

  return v;
}

Report BytecodeAnalyzer::collectStatistics() {
  auto v = reachability();
  PairFrequences pair;
  SingleFrequences single;

  ByteReader reader(bytecode.code, bytecode.codeSize);

  auto prev = decodeInsn(reader).op;
  single[prev]++;

  while (!reader.eof()) {
    if (v.isFirst(reader.pos())) {
      bool sequential = !v.isSecond(reader.pos());
      auto curr = decodeInsn(reader).op;
      single[curr]++;
      if (sequential) pair[pack(prev, curr)]++;
      prev = curr;
    } else
      reader.u8();
  }

  return Report{pair, single};
}
