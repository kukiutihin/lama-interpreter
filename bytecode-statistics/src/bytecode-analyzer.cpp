#include "bytecode-analyzer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <queue>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "bytecode.h"
#include "disassemble.h"
#include "opcodes.h"
#include "reader.h"

std::pair<std::vector<bool>, std::vector<bool>>
BytecodeAnalyzer::reachability() {
  byte_reader reader{bc->code, bc->code_size, 0};

  std::vector<bool> visited(bc->code_size);
  std::vector<bool> isSep(bc->code_size);

  std::queue<std::size_t> entries;
  entries.push(0);
  bytecode_iterator iter{};
  bytecode_pubs_init(&iter, bc);
  public_symbol symb{};
  while (bytecode_pubs_next(&iter, &symb)) entries.push(symb.code_offset);

  while (!entries.empty()) {
    reader.pos = entries.front();
    entries.pop();

    while (reader.pos < reader.size && !visited[reader.pos]) {
      visited[reader.pos] = true;

      auto opcode = reader.data[reader.pos];
      auto begin = reader.pos;

      auto readOn = [&]() {
        auto saved = reader.pos;
        reader.pos = begin + 1;
        auto target = reader_i32(&reader);
        reader.pos = saved;
        return target;
      };

      static uint8_t dummy[1];
      static std::FILE* f = fmemopen(dummy, sizeof(dummy), "w");
      disassemble(f, &reader, bc);

      switch (opcode) {
        case OP_JMP: {
          auto target = readOn();
          isSep[reader.pos] = true;
          isSep[target] = true;
          if (!visited[target]) entries.push(target);
          goto end;
        }

        case OP_CJMP_NZ:
        case OP_CJMP_Z: {
          auto target = readOn();
          isSep[reader.pos] = true;
          isSep[target] = true;
          if (!visited[target]) entries.push(target);
          break;
        }

        case OP_CALL: {
          auto target = readOn();
          if (target >= 0) {
            isSep[reader.pos] = true;
            isSep[target] = true;
          }
          if (target >= 0 && !visited[target]) entries.push(target);
          break;
        }

        case OP_CALLC: {
          isSep[reader.pos] = true;
          break;
        }

        case OP_CLOSURE: {
          auto target = readOn();
          isSep[target] = true;
          if (target >= 0 && !visited[target]) entries.push(target);
          break;
        }

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

  return {visited, isSep};
}

std::vector<Instruction> BytecodeAnalyzer::collectInsns(
    const std::vector<bool>& visited) {
  byte_reader reader{};
  reader_init(&reader, bc->code, bc->code_size);

  std::vector<Instruction> insns;

  static uint8_t dummy[1];
  static std::FILE* f = fmemopen(dummy, sizeof(dummy), "w");

  while (reader.pos < reader.size) {
    if (visited[reader.pos]) {
      auto begin = reader.pos;
      disassemble(f, &reader, bc);
      insns.push_back(
          {std::string_view(reinterpret_cast<const char*>(reader.data + begin),
                            reader.pos - begin),
           begin});
    } else
      reader_u8(&reader);
  }

  return insns;
}

template <typename T>
std::vector<std::pair<T, std::size_t>> count(std::vector<T>& els) {
  std::ranges::sort(els);
  std::vector<std::pair<T, std::size_t>> result;

  if (els.empty()) return {};

  auto landmark = std::move(els[0]);
  std::size_t curr = 1;
  for (auto&& el : els | std::views::drop(1))
    if (el == landmark)
      curr++;
    else {
      result.push_back({std::move(landmark), curr});
      landmark = el;
      curr = 1;
    }
  result.push_back({std::move(landmark), curr});

  std::ranges::sort(result, std::ranges::greater{},
                    &std::pair<T, std::size_t>::second);
  return result;
}

Report BytecodeAnalyzer::countFrequencies() {
  auto [visited, isSep] = reachability();
  auto insns = collectInsns(visited);
  std::vector<std::pair<Instruction, Instruction>> pairs;

  if (!insns.empty()) {
    for (std::size_t i = 1; i < insns.size(); i++) {
      auto prev = insns[i - 1];
      auto curr = insns[i];
      if (!isSep[insns[i].offset]) pairs.push_back({prev, curr});
    }
  }

  return Report{count(insns), count(pairs)};
}

std::string BytecodeAnalyzer::reportToString(const Report& report) {
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

  buffer << "SINGLE INSTRUCTION:\n";

  for (const auto& [insn, freq] : report.single) {
    buffer << readInsn(insn.offset, ':') << " " << freq << "\n";
  }

  buffer << "\nINSTRUCTIONS IN PAIRS:\n";

  for (const auto& [insns, freq] : report.pair) {
    const auto& [insn1, insn2] = insns;
    buffer << readInsn(insn1.offset, ',');
    buffer << readInsn(insn2.offset, ':') << " " << freq << "\n";
  }

  std::fclose(f);
  std::free(insnBuffer);

  return buffer.str();
}
