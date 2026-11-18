#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>

#include "bool-pair-vec.h"
#include "bytecode-loader.h"

using PairFrequences = std::unordered_map<std::uint16_t, std::size_t>;
using SingleFrequences = std::unordered_map<std::uint8_t, std::size_t>;

struct Report {
  PairFrequences pairFrequences;
  SingleFrequences singleFrequences;
};

class BytecodeAnalyzer {
  Bytecode bytecode;

  BoolPairVec reachability();

 public:
  BytecodeAnalyzer(const Bytecode& bc) : bytecode(bc) {}
  Report collectStatistics();

  [[gnu::always_inline]] static std::uint16_t pack(std::uint8_t higher,
                                                   std::uint8_t lower) {
    return (std::uint16_t)0 | (higher << 8) | lower;
  }

  [[gnu::always_inline]] static std::pair<std::uint8_t, std::uint8_t> unpack(
      std::uint16_t value) {
    return std::pair{value >> 8, value & (std::uint8_t)255};
  }
};
