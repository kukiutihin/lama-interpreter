#include "bytecode-reader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include "app-runtime.h"

static const std::uint32_t MAGIC = 0x614D614C;  // LaMa
static const std::int32_t VERSION = 1;

Bytecode BytecodeReader::readBytecode() {
  struct stat st;
  if (fstat(fd, &st) < 0)
    failure("failed to read from file: (fstat) %s\n", strerror(errno));

  // remember data ptr in class to call munmap in destructor
  data = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data == MAP_FAILED)
    failure("failed to read from file: (mmap) %s\n", strerror(errno));
  dataSize = st.st_size;

  if (st.st_size < sizeof(BytecodeHeader))
    failure("corrupted bytecode: file smaller than required header\n");

  BytecodeHeader header;
  memcpy(&header, data, sizeof(BytecodeHeader));

  if (header.magic != MAGIC)
    failure("unknown magic: %s , 'LaMa' expected\n",
            reinterpret_cast<char*>(header.magic));

  if (header.version != VERSION)
    failure("unknown bytecode version: %d , version 1 expected\n",
            header.version);

  std::size_t minSize = sizeof(BytecodeHeader) + header.constPoolSize +
                        (header.importsCount * 4) +
                        (header.publicSymbolsCount * 9) + 1;  // OP_EOF

  if (dataSize < minSize) {
    failure("corrupted bytecode: file truncated\n");
  }

  auto bytes = reinterpret_cast<std::uint8_t*>(data);
  bytes += sizeof(BytecodeHeader);
  std::uint8_t* constPool = bytes;
  bytes += header.constPoolSize;

  // skip imports
  bytes += header.importsCount * 4;

  // skip publicSymbols
  bytes += header.publicSymbolsCount * 9;

  return Bytecode{header, constPool, bytes};
}
