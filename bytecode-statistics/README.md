`lamastat` collects and prints the frequencies of instruction sequences in the given file.

### Build and run
```bash
cmake -B build 
cmake --build build --target lamastat
./build/bytecode-statistics/lamastat <bytecode file>
```

The program requires Lama repository cloned in root, operation demonstrated in CI.

### Tests

To build and run bytecode-statistics tests only:
```bash
cmake -B build 
cmake --build build --target lamastat-tests
./build/bytecode-statistics/tests/lamastat-tests
```
