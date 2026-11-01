#include "bytecode-reader.h"

#include <cstring>
#include <fstream>

#include "app-runtime.h"

static const char* MAGIC = "LaMa";
static const std::size_t MAGIC_SIZE = 4;

Bytecode BytecodeReader::readFromFile(std::ifstream& file) {
  BytecodeHeader header;
  file.read(reinterpret_cast<char*>(&header), sizeof(BytecodeHeader));

  if (!memcmp(reinterpret_cast<const void*>(header.magic),
              reinterpret_cast<const void*>(MAGIC), MAGIC_SIZE))
    failure("unknown magic: %s , 'LaMa' expected\n",
            reinterpret_cast<char*>(header.magic));

  return Bytecode{header};
}
