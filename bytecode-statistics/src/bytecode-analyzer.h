#pragma once

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

extern "C" {
#include "bytecode.h"
}

struct Instruction {
  std::string_view bytes;
  std::size_t offset;

  auto operator<=>(const Instruction& other) const {
    return bytes <=> other.bytes;
  }

  auto operator==(const Instruction& other) const {
    return bytes == other.bytes;
  }
};

using SingleWithFrequency = std::pair<Instruction, std::size_t>;
using PairWithFrequency =
    std::pair<std::pair<Instruction, Instruction>, std::size_t>;

struct Report {
  std::vector<SingleWithFrequency> single;
  std::vector<PairWithFrequency> pair;
};

class BytecodeAnalyzer {
  const bytecode* bc;

 public:
  BytecodeAnalyzer(const bytecode* bc) : bc(bc) {}
  std::pair<std::vector<bool>, std::vector<bool>> reachability();
  std::vector<Instruction> collectInsns(const std::vector<bool>& visited);
  Report countFrequencies();
  std::string reportToString(const Report&);
};
