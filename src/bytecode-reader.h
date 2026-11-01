#pragma once

#include <cstdint>
#include <fstream>

#pragma pack(push, 1)
struct BytecodeHeader {
  std::uint32_t magic;
  std::int32_t version;
  std::int32_t constPoolSize;
  std::int32_t globalsCount;
  std::int32_t importsCount;
  std::int32_t publicSymbolsCount;
};
#pragma pack(pop)

struct PublicSymbol {
  std::int32_t nameOffset;
  std::int32_t codeOffset;
  std::uint8_t flag;
};

struct Bytecode {
  BytecodeHeader header;
  std::uint8_t* constantPool;
  std::int32_t* imports;
  PublicSymbol* symbols;
  std::uint8_t* code;
};

class BytecodeReader {
 public:
  Bytecode readFromFile(std::ifstream& file);
};
