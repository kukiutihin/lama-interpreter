#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include <string_view>

#include "disassemble.h"

extern "C" {
#include "bytecode.h"

extern _Noreturn void failure(char* s, ...);
}

struct Instruction {
  std::uint32_t offset;
};

struct InstructionPair {
  std::uint32_t offset;
};

using SingleFrequencies = std::vector<std::pair<Instruction, std::uint32_t>>;
using PairFrequencies = std::vector<std::pair<InstructionPair, std::uint32_t>>;

class BytecodeView {
  const bytecode* bc;

 public:
  BytecodeView(const bytecode* bc) : bc(bc) {}
  std::uint32_t insnLen(std::uint32_t offset) const noexcept {
    static uint8_t dummy[1];
    static std::FILE* f = fmemopen(dummy, sizeof(dummy), "w");

    byte_reader reader{bc->code, bc->code_size, offset};
    return disassemble(f, &reader, bc);
  }

  std::string_view bytes(Instruction insn) const noexcept {
    const auto* p = bc->code + insn.offset;
    return {reinterpret_cast<const char*>(p), insnLen(insn.offset)};
  }

  std::string_view bytes(InstructionPair pair) const noexcept {
    const auto* p = bc->code + pair.offset;
    std::size_t l1 = insnLen(pair.offset);
    std::size_t l2 = insnLen(pair.offset + static_cast<std::uint32_t>(l1));
    return {reinterpret_cast<const char*>(p), l1 + l2};
  }
};

struct BytecodeLess {
  BytecodeView view;

  template <typename T>
  bool operator()(T a, T b) {
    return view.bytes(a) < view.bytes(b);
  }
};

class BytecodeAnalyzer {
  const bytecode* bc;

  std::vector<uint32_t> preparePublics(std::vector<bool>& isSep);

 public:
  BytecodeAnalyzer(const bytecode* bc) : bc(bc) {}

  std::pair<std::vector<bool>, std::vector<bool>> reachability();

  SingleFrequencies countSingleFrequencies(const std::vector<bool>& visited);
  PairFrequencies countPairFrequencies(std::vector<bool>&& visited,
                                       std::vector<bool>&& isSep);

  void printStatistics();
};
