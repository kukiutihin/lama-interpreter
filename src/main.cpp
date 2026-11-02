#include "app-runtime.h"
#include "bytecode-reader.h"
#include "virtual-machine.h"

int main(int argc, char* argv[]) {
  if (argc != 2) failure("expected usage: app <path_to_bytecode>\n");

  BytecodeReader reader(argv[1]);
  auto bytecode = reader.readBytecode();

  GC gc;
  VirtualMachine vm(bytecode.constantPool, bytecode.header.globalsCount, gc);
  vm.interpret(bytecode.code);
}
