## LaMa interpreter

### Build 
Interpreter depends on the LaMa runtime, clone it to the root then use `make`.

### Tests
Tests use the LaMa compiler for comparison, so `lamac` must be available in PATH.

To run correctness tests use `./regression`, example:
```sh
./regression.sh

TEST: test001 PASSED

TEST: test002 PASSED
...
```

To run performance comparison test for interpreters use `./performance`, example:
```sh
```