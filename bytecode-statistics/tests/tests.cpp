#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

#include "bytecode-analyzer.h"

extern "C" {
#include "opcodes.h"
}

namespace {

struct CodeBuilder {
  std::vector<uint8_t> buf;

  std::size_t pos() const { return buf.size(); }

  std::size_t emitOp(uint8_t op) {
    std::size_t off = buf.size();
    buf.push_back(op);
    return off;
  }

  void emitI32(int32_t val) {
    uint8_t bytes[4];
    std::memcpy(bytes, &val, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
  }

  std::size_t emitConst(int32_t v) {
    auto off = emitOp(OP_CONST);
    emitI32(v);
    return off;
  }

  std::size_t emitJmp(int32_t target) {
    auto off = emitOp(OP_JMP);
    emitI32(target);
    return off;
  }

  std::size_t emitCjmpZ(int32_t target) {
    auto off = emitOp(OP_CJMP_Z);
    emitI32(target);
    return off;
  }

  std::size_t emitCall(int32_t target, int32_t nargs) {
    auto off = emitOp(OP_CALL);
    emitI32(target);
    emitI32(nargs);
    return off;
  }

  std::size_t emitClosure(
      int32_t target,
      const std::vector<std::pair<uint8_t, int32_t>>& captures = {}) {
    auto off = emitOp(OP_CLOSURE);
    emitI32(target);
    emitI32(static_cast<int32_t>(captures.size()));
    for (const auto& [kind, idx] : captures) {
      buf.push_back(kind);
      emitI32(idx);
    }
    return off;
  }
};

bytecode makeBytecode(const std::vector<uint8_t>& code,
                      const std::vector<uint8_t>& pubs = {},
                      std::size_t pubsLen = 0, const char* strtab = "",
                      std::size_t strtabSize = 0) {
  bytecode bc{};
  bc.version = 1;
  bc.code = code.data();
  bc.code_size = code.size();
  bc.pubs = pubs.data();
  bc.pubs_len = pubsLen;
  bc.string_table = strtab;
  bc.string_table_size = strtabSize;
  return bc;
}

std::optional<std::uint32_t> findFreqForOpcode(
    const SingleFrequencies& freqs, const std::vector<std::uint8_t>& code,
    std::uint8_t expectedOpcode) {
  for (const auto& [instr, count] : freqs) {
    if (code[instr.offset] == expectedOpcode) {
      return count;
    }
  }
  return std::nullopt;
}

std::optional<std::uint32_t> findFreqForInsn(
    const SingleFrequencies& freqs, const std::vector<std::uint8_t>& code,
    std::size_t targetOffset, std::size_t byteLen) {
  for (const auto& [instr, count] : freqs) {
    if (instr.offset + byteLen <= code.size() &&
        std::equal(code.begin() + instr.offset,
                   code.begin() + instr.offset + byteLen,
                   code.begin() + targetOffset)) {
      return count;
    }
  }
  return std::nullopt;
}

}  // namespace

TEST_CASE("reachability: linear code stops at END and ignores dead code") {
  CodeBuilder cb;
  auto o0 = cb.emitOp(OP_DUP);
  auto o1 = cb.emitOp(OP_DROP);
  auto o2 = cb.emitOp(OP_END);
  auto dead = cb.emitOp(OP_DUP);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, isSep] = analyzer.reachability();

  REQUIRE(std::ranges::count(visited, true) == 3);
  REQUIRE(visited[o0]);
  REQUIRE(visited[o1]);
  REQUIRE(visited[o2]);
  REQUIRE_FALSE(visited[dead]);
}

TEST_CASE("reachability: JMP skips dead code and marks separator") {
  CodeBuilder cb;
  auto jmpOff = cb.emitJmp(6);
  auto deadOff = cb.emitOp(OP_DUP);
  auto targetOff = cb.emitOp(OP_DROP);
  auto endOff = cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  REQUIRE(deadOff == 5);
  REQUIRE(targetOff == 6);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, isSep] = analyzer.reachability();

  REQUIRE(std::ranges::count(visited, true) == 3);
  REQUIRE(visited[jmpOff]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[targetOff]);
  REQUIRE(visited[endOff]);
  REQUIRE(isSep[targetOff]);
}

TEST_CASE("reachability: CJMP_Z visits both fall-through and target") {
  CodeBuilder cb;
  // 0: CJMP_Z 8
  // 5: DUP
  // 6: END
  // 7: DUP (dead)
  // 8: DROP
  // 9: END
  auto cjmpOff = cb.emitCjmpZ(8);
  auto fallOff = cb.emitOp(OP_DUP);
  auto end1Off = cb.emitOp(OP_END);
  auto deadOff = cb.emitOp(OP_DUP);
  auto targetOff = cb.emitOp(OP_DROP);
  auto end2Off = cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, isSep] = analyzer.reachability();

  REQUIRE(std::ranges::count(visited, true) == 5);
  REQUIRE(visited[cjmpOff]);
  REQUIRE(visited[fallOff]);
  REQUIRE(visited[end1Off]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[targetOff]);
  REQUIRE(visited[end2Off]);
  REQUIRE(isSep[targetOff]);
}

TEST_CASE("reachability: CALL visits callee and continues after call") {
  CodeBuilder cb;
  // 0:  CALL target=11, args=1
  // 9:  DROP
  // 10: END
  // 11: DUP
  // 12: END
  auto callOff = cb.emitCall(11, 1);
  auto afterCall = cb.emitOp(OP_DROP);
  auto mainEnd = cb.emitOp(OP_END);
  auto calleeOff = cb.emitOp(OP_DUP);
  auto calleeEnd = cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, isSep] = analyzer.reachability();

  REQUIRE(std::ranges::count(visited, true) == 5);
  REQUIRE(visited[callOff]);
  REQUIRE(visited[afterCall]);
  REQUIRE(visited[mainEnd]);
  REQUIRE(visited[calleeOff]);
  REQUIRE(visited[calleeEnd]);
  REQUIRE(isSep[calleeOff]);
  REQUIRE(isSep[afterCall]);
}

TEST_CASE("reachability: CLOSURE visits closure body and skips dead code") {
  CodeBuilder cb;
  // 0:  CLOSURE target=17, captures=[(1, 0)] -> 1 + 4 + 4 + (1 + 4) = 14 bytes
  // 14: DROP
  // 15: END
  // 16: DUP  (dead code)
  // 17: DUP  (closure body)
  // 18: END
  auto closureOff = cb.emitClosure(17, {{1, 0}});
  auto afterClosure = cb.emitOp(OP_DROP);
  auto mainEnd = cb.emitOp(OP_END);
  auto deadOff = cb.emitOp(OP_DUP);
  auto bodyOff = cb.emitOp(OP_DUP);
  auto bodyEnd = cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  REQUIRE(afterClosure == 14);
  REQUIRE(deadOff == 16);
  REQUIRE(bodyOff == 17);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, isSep] = analyzer.reachability();

  REQUIRE(std::ranges::count(visited, true) == 5);
  REQUIRE(visited[closureOff]);
  REQUIRE(visited[afterClosure]);
  REQUIRE(visited[mainEnd]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[bodyOff]);
  REQUIRE(visited[bodyEnd]);
  REQUIRE(isSep[bodyOff]);
}

TEST_CASE("countSingleFrequencies: finds same zero-operand opcodes") {
  CodeBuilder cb;
  auto o0 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o1 = static_cast<std::uint32_t>(cb.emitOp(OP_DROP));
  auto o2 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o3 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o4 = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[o0] = visited[o1] = visited[o2] = visited[o3] = visited[o4] = true;
  auto freqs = analyzer.countSingleFrequencies(visited);

  REQUIRE(freqs.size() == 3);
  REQUIRE(findFreqForOpcode(freqs, cb.buf, OP_DUP) == 3);
  REQUIRE(findFreqForOpcode(freqs, cb.buf, OP_DROP) == 1);
  REQUIRE(findFreqForOpcode(freqs, cb.buf, OP_END) == 1);

  auto dupIt = std::ranges::find_if(
      freqs, [&](const auto& p) { return cb.buf[p.first.offset] == OP_DUP; });
  REQUIRE(dupIt != freqs.end());
  REQUIRE((dupIt->first.offset == o0 || dupIt->first.offset == o2 ||
           dupIt->first.offset == o3));
}

TEST_CASE(
    "countSingleFrequencies: same opcode with different operands is another "
    "instruction") {
  CodeBuilder cb;
  auto c42_a = static_cast<std::uint32_t>(cb.emitConst(42));  // 5 bytes
  auto c99 = static_cast<std::uint32_t>(cb.emitConst(99));    // 5 bytes
  auto c42_b = static_cast<std::uint32_t>(cb.emitConst(42));  // 5 bytes
  auto endOff = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[c42_a] = visited[c99] = visited[c42_b] = visited[endOff] = true;
  auto freqs = analyzer.countSingleFrequencies(visited);

  REQUIRE(freqs.size() == 3);
  REQUIRE(findFreqForInsn(freqs, cb.buf, c42_a, 5) == 2);
  REQUIRE(findFreqForInsn(freqs, cb.buf, c99, 5) == 1);
  REQUIRE(findFreqForInsn(freqs, cb.buf, endOff, 1) == 1);
}

std::optional<std::uint32_t> findFreqForPair(
    const PairFrequencies& freqs, const std::vector<std::uint8_t>& code,
    std::size_t targetOffset, std::size_t totalByteLen) {
  for (const auto& [pair, count] : freqs) {
    if (pair.offset + totalByteLen <= code.size() &&
        std::equal(code.begin() + pair.offset,
                   code.begin() + pair.offset + totalByteLen,
                   code.begin() + targetOffset)) {
      return count;
    }
  }
  return std::nullopt;
}

TEST_CASE("countPairFrequencies") {
  CodeBuilder cb;
  //  DUP, DROP, DUP, DROP, DUP, END
  //   (DUP, DROP) x2  
  //   (DROP, DUP) x2  
  //   (DUP, END)  x1  
  auto o0 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o1 = static_cast<std::uint32_t>(cb.emitOp(OP_DROP));
  auto o2 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o3 = static_cast<std::uint32_t>(cb.emitOp(OP_DROP));
  auto o4 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o5 = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[o0] = visited[o1] = visited[o2] = visited[o3] = visited[o4] =
      visited[o5] = true;
  std::vector<bool> isSep(cb.buf.size(), false);

  auto freqs = analyzer.countPairFrequencies(std::move(visited), std::move(isSep));

  REQUIRE(freqs.size() == 3);
  REQUIRE(findFreqForPair(freqs, cb.buf, o0, 2) == 2);  // DUP, DROP
  REQUIRE(findFreqForPair(freqs, cb.buf, o1, 2) == 2);  // DROP, DUP
  REQUIRE(findFreqForPair(freqs, cb.buf, o4, 2) == 1);  // DUP, END
  REQUIRE(freqs.back().second == 1);
}

TEST_CASE("countPairFrequencies: distinguishes multi-byte instructions by operands") {
  CodeBuilder cb;
  // (CONST 10, DUP) x2, (CONST 20, DUP) x1
  auto c10_a = static_cast<std::uint32_t>(cb.emitConst(10));
  auto dup1 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto c20 = static_cast<std::uint32_t>(cb.emitConst(20)); 
  auto dup2 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto c10_b = static_cast<std::uint32_t>(cb.emitConst(10));
  auto dup3 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto endOff = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[c10_a] = visited[dup1] = visited[c20] = visited[dup2] =
      visited[c10_b] = visited[dup3] = visited[endOff] = true;

  auto freqs = analyzer.countPairFrequencies(
      std::move(visited), std::vector<bool>(cb.buf.size(), false));

  REQUIRE(findFreqForPair(freqs, cb.buf, c10_a, 6) == 2);
  REQUIRE(findFreqForPair(freqs, cb.buf, c20, 6) == 1);
  REQUIRE(findFreqForPair(freqs, cb.buf, dup3, 2) == 1);  // DUP, END
}

TEST_CASE("countPairFrequencies: isSep on second element prevents forming a pair") {
  CodeBuilder cb;
  // 0: DUP
  // 1: DROP  <-- isSep[1] = true
  // 2: SWAP
  // 3: END
  auto o0 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  auto o1 = static_cast<std::uint32_t>(cb.emitOp(OP_DROP));
  auto o2 = static_cast<std::uint32_t>(cb.emitOp(OP_SWAP));
  auto o3 = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[o0] = visited[o1] = visited[o2] = visited[o3] = true;

  std::vector<bool> isSep(cb.buf.size(), false);
  isSep[o1] = true;

  auto freqs = analyzer.countPairFrequencies(std::move(visited), std::move(isSep));

  REQUIRE(freqs.size() == 2);
  REQUIRE(findFreqForPair(freqs, cb.buf, o0, 2) == std::nullopt);  // DUP, DROP
  REQUIRE(findFreqForPair(freqs, cb.buf, o1, 2) == 1);             // DROP, SWAP
  REQUIRE(findFreqForPair(freqs, cb.buf, o2, 2) == 1);             // SWAP, END
}

TEST_CASE("countPairFrequencies: unvisited gap breaks adjacency") {
  CodeBuilder cb;
  // 0: DUP   
  // 1: SWAP  (dead)
  // 2: DROP  
  // 3: END   
  auto o0 = static_cast<std::uint32_t>(cb.emitOp(OP_DUP));
  cb.emitOp(OP_SWAP);
  auto o2 = static_cast<std::uint32_t>(cb.emitOp(OP_DROP));
  auto o3 = static_cast<std::uint32_t>(cb.emitOp(OP_END));
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);

  std::vector<bool> visited(cb.buf.size(), false);
  visited[o0] = visited[o2] = visited[o3] = true;

  auto freqs = analyzer.countPairFrequencies(
      std::move(visited), std::vector<bool>(cb.buf.size(), false));

  // Only (DROP, END) is adjacent and visited
  REQUIRE(freqs.size() == 1);
  REQUIRE(findFreqForPair(freqs, cb.buf, o2, 2) == 1);
}
