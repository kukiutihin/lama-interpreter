#include <algorithm>
#include <print>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app-runtime.h"
#include "bytecode-analyzer.h"
#include "bytecode-loader.h"
#include "tokens.h"

template <typename T>
std::vector<std::pair<T, std::size_t>> mapToVec(
    const std::unordered_map<T, std::size_t>& m) {
  std::vector<std::pair<T, std::size_t>> vec(m.begin(), m.end());

  std::sort(vec.begin(), vec.end(), [](const auto& pairA, const auto& pairB) {
    return pairA.second > pairB.second;
  });
  return vec;
}

int main(int argc, char* argv[]) {
  if (argc != 2) failure("expected usage: app <path_to_bytecode>\n");

  BytecodeLoader reader(argv[1]);
  auto bytecode = reader.loadBytecode();

  BytecodeAnalyzer analyzer(bytecode);
  auto report = analyzer.analyze();

  auto single = mapToVec(report.singleFrequences);
  auto pair = mapToVec(report.pairFrequences);

  std::stringstream buffer;

  for (const auto& [op, freq] : single)
    buffer << opToString((Token)op) << ": " << freq << "\n";

  for (const auto& [opPair, freq] : pair) {
    auto [fOp, sOp] = BytecodeAnalyzer::unpack(opPair);
    buffer << "(" << opToString((Token)fOp) << " -> " << opToString((Token)sOp)
           << "): " << freq << "\n";
  }

  std::println("{}", buffer.str());
}
