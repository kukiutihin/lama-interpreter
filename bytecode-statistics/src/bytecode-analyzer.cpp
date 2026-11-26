#include "bytecode-analyzer.h"

#include <complex.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <print>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "bytecode.h"
#include "disassemble.h"
#include "reader.h"

std::vector<std::uint32_t> BytecodeAnalyzer::preparePublics(
    std::vector<bool>& isSep /* also used as visited */) {
  std::vector<std::uint32_t> entries;
  entries.reserve(bc->pubs_len);
  entries.push_back(0);

  bytecode_iterator iter{};
  bytecode_pubs_init(&iter, bc);
  public_symbol symb{};

  while (bytecode_pubs_next(&iter, &symb)) {
    if (symb.code_offset < 0 || symb.code_offset >= bc->code_size)
      failure("Incorrect public symbol offset: %d, code size: %d",
              symb.code_offset, bc->code_size);
    if (!isSep[symb.code_offset]) {
      isSep[symb.code_offset] = true;
      entries.push_back(symb.code_offset);
    }
  }
  return entries;
}

std::pair<std::vector<bool>, std::vector<bool>>
BytecodeAnalyzer::reachability() {
  byte_reader reader{bc->code, bc->code_size, 0};
  std::vector<bool> visited(bc->code_size);
  std::vector<bool> isSep(bc->code_size);

  auto entries = preparePublics(isSep);

  auto pop = [&]() {
    auto el = entries.back();
    entries.pop_back();
    return el;
  };

  while (!entries.empty()) {
    reader.pos = pop();

    while (reader.pos < reader.size && !visited[reader.pos]) {
      visited[reader.pos] = true;
      auto info = getInfo(&reader, bc);

      if (info.hasTarget && info.target >= 0) {
        if (!visited[info.target]) entries.push_back(info.target);
        if (info.addsEntryPoint) isSep[info.target] = true;
      }

      if (info.breaksFlow) isSep[reader.pos] = true;
      if (info.isEof) break;
    }
  }

  return {visited, isSep};
}

void BytecodeAnalyzer::printStatistics() {
  auto [visited, isSep] = reachability();

  std::stringstream buffer;
  char* insnBuffer = nullptr;
  size_t size = 0;
  std::FILE* f = open_memstream(&insnBuffer, &size);

  byte_reader reader;
  reader_init(&reader, bc->code, bc->code_size);

  auto readInsn = [&](std::size_t pos, char endChar) -> std::string_view {
    std::rewind(f);
    reader.pos = pos;

    disassemble(f, &reader, bc);
    std::fflush(f);

    if (size > 0 && insnBuffer[size - 1] == '\n')
      insnBuffer[size - 1] = endChar;
    return std::string_view(insnBuffer, size);
  };

  auto single = countSingleFrequencies(visited);
  buffer << "SINGLE INSTRUCTION:\n";
  for (auto&& [insn, freq] : single) {
    buffer << readInsn(insn.offset, ':') << " " << freq << "\n";
  }
  std::println("{}", buffer.str());
  buffer.str("");

  auto pairs = countPairFrequencies(std::move(visited), std::move(isSep));
  buffer << "\nINSTRUCTIONS IN PAIRS:\n";
  for (auto&& [pair, freq] : pairs) {
    buffer << readInsn(pair.offset, ',') << " ";
    buffer << readInsn(reader.pos, ':') << " " << freq << "\n";
  }

  std::fclose(f);
  std::free(insnBuffer);

  std::println("{}", buffer.str());
}

template <typename T>
std::vector<std::pair<T, std::uint32_t>> count(std::vector<T>&& insns,
                                               const bytecode* bc) {
  if (insns.empty()) return {};
  std::vector<std::pair<T, std::uint32_t>> result;

  BytecodeView view(bc);
  std::ranges::sort(insns, BytecodeLess{view});

  auto landmark = std::move(insns[0]);
  std::size_t curr = 1;
  for (auto&& el : insns | std::views::drop(1))
    if (view.bytes(el) == view.bytes(landmark))
      curr++;
    else {
      result.push_back({std::move(landmark), curr});
      landmark = el;
      curr = 1;
    }
  result.push_back({std::move(landmark), curr});

  std::ranges::sort(result, std::ranges::greater{},
                    &std::pair<T, std::uint32_t>::second);
  return result;
}

SingleFrequencies BytecodeAnalyzer::countSingleFrequencies(
    const std::vector<bool>& visited) {
  if (visited.empty()) return {};

  std::vector<Instruction> insns;
  for (std::uint32_t i = 0; i < visited.size(); i++)
    if (visited[i]) insns.push_back(Instruction{i});

  return count(std::move(insns), bc);
}

PairFrequencies BytecodeAnalyzer::countPairFrequencies(
    std::vector<bool>&& visited, std::vector<bool>&& isSep) {
  if (visited.empty()) return {};

  BytecodeView view(bc);
  std::vector<InstructionPair> insns;

  std::uint32_t prev = 0;
  while (prev < visited.size() && !visited[prev]) prev++;
  if (prev == visited.size()) return {};

  bool hasPrev = true;
  std::uint32_t i = prev + view.insnLen(prev);

  while (i < visited.size()) {
    if (visited[i]) {
      if (!isSep[i] && hasPrev) insns.push_back(InstructionPair{prev});

      prev = i;
      hasPrev = true;
      i += view.insnLen(i);
    } else {
      hasPrev = false;
      i++;
    }
  }

  return count(std::move(insns), bc);
}
