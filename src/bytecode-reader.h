#pragma once

#include <fcntl.h>
#include <sys/mman.h>

#include <cstdint>
#include <cstring>
#include <fstream>

#include "app-runtime.h"

struct BytecodeHeader {
  std::uint32_t magic;
  std::int32_t version;
  std::int32_t constPoolSize;
  std::int32_t globalsCount;
  std::int32_t importsCount;
  std::int32_t publicSymbolsCount;
};

struct Bytecode {
  BytecodeHeader header;
  std::uint8_t* constantPool;
  std::size_t codeStartOffset;
  std::uint8_t* code;
};

class BytecodeReader {
  int fd;
  void* data = nullptr;
  std::size_t dataSize;

 public:
  BytecodeReader(const char* path) {
    fd = open(path, O_RDONLY);
    if (fd < 0) failure("failed to open %s: %s\n", path, strerror(errno));
  }

  ~BytecodeReader() {
    if (data) munmap(data, dataSize);
  }

  Bytecode readBytecode();
};
