#include <cstddef>
#include <cstring>
#include <fstream>

#include "bytecode-reader.h"
#include "virtual-machine.h"

int main(int argc, char* argv[]) {
  if (argc != 2) failure("expected usage: app <path_to_bytecode>\n");

  std::ifstream bytecodeFile(argv[1], std::ios::binary);
  if (!bytecodeFile) failure("failed to open bytecode file: %s\n", argv[1]);

  BytecodeReader reader;
  auto bytecode = reader.readFromFile(bytecodeFile);

  GC gc;
  VirtualMachine vm(bytecode.constantPool, bytecode.header.globalsCount, gc);
  vm.interpret(bytecode.code);
}
