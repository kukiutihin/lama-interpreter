`lamastat` collects and prints the frequencies of opcode sequences in the given file

### Build and run
```bash
cmake -B build 
cmake --build build --target lamastat
./build/bytecode-statistics/lamastat <bytecode file>
```

### Example
```
OP_DROP: 31
OP_DUP: 28
OP_CONST: 28
...

(OP_CONST -> OP_ELEM): 21
(OP_DUP -> OP_CONST): 18
(OP_DROP -> OP_DUP): 11
...
```

Frequences are not count in the dead code. To check correctness of that run 
```sh
./dceCorrectness
```
