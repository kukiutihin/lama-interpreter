#include "app-runtime.h"
#include "bytecode-loader.h"
#include "virtual-machine.h"

int main(int argc, char* argv[]) {
  if (argc != 2) failure("expected usage: app <path_to_bytecode>\n");

  BytecodeLoader reader(argv[1]);
  auto bytecode = reader.loadBytecode();

  GC gc;
  VirtualMachine vm(bytecode, gc);
  vm.interpret();
}
