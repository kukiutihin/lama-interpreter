#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "bytecode-loader.h"

using PairFrequences = std::unordered_map<std::uint16_t, std::size_t>;
using SingleFrequences = std::unordered_map<std::uint8_t, std::size_t>;

struct Report {
  PairFrequences pairFrequences;
  SingleFrequences singleFrequences;
};

class BytecodeAnalyzer {
  Bytecode bytecode;

  std::unordered_set<std::size_t> getSeparators();

 public:
  BytecodeAnalyzer(const Bytecode& bc) : bytecode(bc) {}
  Report analyze();

  static std::uint16_t pack(std::uint8_t higher, std::uint8_t lower) {
    return (std::uint16_t)0 | (higher << 8) | lower;
  }

  static std::pair<std::uint8_t, std::uint8_t> unpack(std::uint16_t value) {
    return std::pair{value >> 8, value & (std::uint8_t)255};
  }
};
