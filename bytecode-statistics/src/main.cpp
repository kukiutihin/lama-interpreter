#include <cstdio>
#include <print>

#include "bytecode-analyzer.h"

extern "C" {
#include "bytecode.h"
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::println(stderr, "expected usage: app <path_to_bytecode>\n");
    return 1;
  }

  auto* bytecode = bytecode_load(argv[1]);

  BytecodeAnalyzer analyzer(bytecode);
  auto report = analyzer.countFrequencies();
  std::println("{}", analyzer.reportToString(report));

  bytecode_free(bytecode);
}
