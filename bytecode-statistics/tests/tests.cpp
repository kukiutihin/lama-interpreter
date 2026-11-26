#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <string_view>
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

  REQUIRE(visited[jmpOff]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[targetOff]);
  REQUIRE(visited[endOff]);

  REQUIRE(isSep[targetOff]);
}

TEST_CASE("reachability: CJMP_Z visits both fall-through and target") {
  CodeBuilder cb;
  // 0: CJMP_Z 7
  // 5: DUP
  // 6: END
  // 7: DUP
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

  REQUIRE(visited[cjmpOff]);
  REQUIRE(visited[fallOff]);
  REQUIRE(visited[end1Off]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[targetOff]);
  REQUIRE(visited[end2Off]);

  REQUIRE(isSep[fallOff]);
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

  REQUIRE(visited[closureOff]);
  REQUIRE(visited[afterClosure]);
  REQUIRE(visited[mainEnd]);
  REQUIRE_FALSE(visited[deadOff]);
  REQUIRE(visited[bodyOff]);
  REQUIRE(visited[bodyEnd]);
  REQUIRE(isSep[bodyOff]);
}

TEST_CASE("collectInsns") {
  CodeBuilder cb;
  auto o0 = cb.emitConst(42);   // 5 bytes
  auto o1 = cb.emitOp(OP_DUP);  // 1 byte
  auto o2 = cb.emitOp(OP_END);  // 1 byte
  auto dead = cb.emitOp(OP_DROP);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto [visited, _] = analyzer.reachability();
  auto insns = analyzer.collectInsns(visited);

  REQUIRE(insns.size() == 3);

  REQUIRE(insns[0].offset == o0);
  REQUIRE(insns[0].bytes.size() == 5);
  REQUIRE(static_cast<uint8_t>(insns[0].bytes[0]) == OP_CONST);

  REQUIRE(insns[1].offset == o1);
  REQUIRE(insns[1].bytes.size() == 1);
  REQUIRE(static_cast<uint8_t>(insns[1].bytes[0]) == OP_DUP);

  REQUIRE(insns[2].offset == o2);
  REQUIRE(insns[2].bytes.size() == 1);
  REQUIRE(static_cast<uint8_t>(insns[2].bytes[0]) == OP_END);

  (void)dead;
}

TEST_CASE("countFrequencies: single") {
  CodeBuilder cb;
  // 3x DUP, 2x DROP, 1x CONST 7, 1x END
  cb.emitOp(OP_DUP);
  cb.emitOp(OP_DUP);
  cb.emitOp(OP_DUP);
  cb.emitOp(OP_DROP);
  cb.emitOp(OP_DROP);
  cb.emitConst(7);
  cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto report = analyzer.countFrequencies();

  REQUIRE(report.single.size() == 4);
  REQUIRE(static_cast<uint8_t>(report.single[0].first.bytes[0]) == OP_DUP);
  REQUIRE(report.single[0].second == 3);

  REQUIRE(static_cast<uint8_t>(report.single[1].first.bytes[0]) == OP_DROP);
  REQUIRE(report.single[1].second == 2);
}

TEST_CASE("countFrequencies: pairs do not cross basic block boundaries") {
  CodeBuilder cb;
  // block 1: DUP, DROP, JMP 8
  // block 2 (offset 8): DUP, DROP, END
  cb.emitOp(OP_DUP);
  cb.emitOp(OP_DROP);
  cb.emitJmp(7);  
  cb.emitOp(OP_DUP);
  cb.emitOp(OP_DROP);
  cb.emitOp(OP_END);
  cb.emitOp(OP_EOF);

  auto bc = makeBytecode(cb.buf);
  BytecodeAnalyzer analyzer(&bc);
  auto report = analyzer.countFrequencies();

  // (DUP, DROP) appears twice; (JMP, DUP) must not appear because offset 7 is a
  // separator
  REQUIRE_FALSE(report.pair.empty());
  REQUIRE(static_cast<uint8_t>(report.pair[0].first.first.bytes[0]) == OP_DUP);
  REQUIRE(static_cast<uint8_t>(report.pair[0].first.second.bytes[0]) ==
          OP_DROP);
  REQUIRE(report.pair[0].second == 2);

  for (const auto& [p, _] : report.pair) {
    REQUIRE_FALSE((static_cast<uint8_t>(p.first.bytes[0]) == OP_JMP &&
                   static_cast<uint8_t>(p.second.bytes[0]) == OP_DUP));
  }
}
