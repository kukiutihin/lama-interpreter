Interpreter depends on the Lama runtime, clone Lama repository to the root.

### Build and run
```bash
cmake -B build 
cmake --build build --target lamaj
./build/interpreter/lamaj <bytecode file>
```

### Tests
Tests use the Lama compiler for comparison, so `lamac` must be available in PATH.

To run correctness tests use `./regression`, example:
```sh
./regression.sh

REGRESSION:
TEST: test001 PASSED
TEST: test002 PASSED
...
```

To run performance comparison test for interpreters use `./performance`, example:
```sh
# 100 iterations
lamac -i: 0.096s
lamaSkib: 0.021s
WINNER: lamaSkib (4.48x faster)

# 1000 iterations
lamac -i: 3.439s
lamaSkib: 1.423s
WINNER: lamaSkib (2.42x faster)
```
