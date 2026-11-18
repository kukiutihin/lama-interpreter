#include "bytecode-analyzer.h"

#include <unordered_set>

#include "byte-reader.h"
#include "disasm.h"
#include "tokens.h"

std::unordered_set<std::size_t> BytecodeAnalyzer::getSeparators() {
  ByteReader reader(bytecode.code, bytecode.codeSize);
  std::unordered_set<std::size_t> seps;

  while (!reader.eof()) {
    auto decoded = decodeInsn(reader);
    switch (decoded.op) {
      case OP_JMP:
      case OP_CJMP_NZ:
      case OP_CJMP_Z:
        seps.insert(reader.pos());
        seps.insert(decoded.target);
        break;

      case OP_CALL: {
        if (decoded.target >= 0) {
          seps.insert(reader.pos());
          seps.insert(decoded.target);
        }
        break;
      }

      default:
        break;
    }
  }
  return seps;
}

Report BytecodeAnalyzer::analyze() {
  auto seps = getSeparators();
  PairFrequences pair;
  SingleFrequences single;

  ByteReader reader(bytecode.code, bytecode.codeSize);

  auto prev = decodeInsn(reader).op;
  single[prev]++;

  while (!reader.eof()) {
    bool sequential = !seps.contains(reader.pos());
    auto curr = decodeInsn(reader).op;
    single[curr]++;
    if (sequential) pair[pack(prev, curr)]++;
    prev = curr;
  }

  return Report{pair, single};
}
