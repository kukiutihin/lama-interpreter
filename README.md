### Lama interpreter

A blazingly fast, zero-overhead and production ready bytecode interpreter and static analysis toolkit for the Lama programming language.

Modules:
- `/interpreter`: iterative Lama bytecode interpreter.
- `/bytecode-statistics`: tool for collecting the frequency of opcode occurrence.

### Testing 
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build 
ctest --test-dir build --output-on-failure
```
