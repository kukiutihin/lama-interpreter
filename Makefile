CXX = clang++
CC = clang
FLAGS = -std=gnu++23 -O3 -Wno-writable-strings

BIN = app
RUNTIME_DIR = Lama/runtime
SRC = src

all: $(BIN)


gc.o: $(RUNTIME_DIR)/gc.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

gc-debug.o: $(RUNTIME_DIR)/gc.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value -g -O0


runtime.o: $(RUNTIME_DIR)/runtime.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

runtime-debug.o: $(RUNTIME_DIR)/runtime.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value -g -O0


virtual-machine.o: $(SRC)/virtual-machine.cpp
	$(CXX) $(FLAGS) $^ -c -o $@ -I $(RUNTIME_DIR) 

virtual-machine-debug.o: $(SRC)/virtual-machine.cpp
	$(CXX) $(FLAGS) $^ -c -o $@ -g -I $(RUNTIME_DIR) -O0


bytecode-reader.o: $(SRC)/bytecode-reader.cpp
	$(CXX) $(FLAGS) $^ -c -o $@ -I $(RUNTIME_DIR) 

bytecode-reader-debug.o: $(SRC)/bytecode-reader.cpp
	$(CXX) $(FLAGS) $^ -c -o $@ -g -I $(RUNTIME_DIR) -O0


$(BIN): $(SRC)/main.cpp \
virtual-machine.o bytecode-reader.o runtime.o gc.o
	$(CXX) $(FLAGS) $^ -o $@ -I $(RUNTIME_DIR) 

$(BIN)-debug: $(SRC)/main.cpp \
bytecode-reader-debug.o virtual-machine-debug.o runtime-debug.o gc-debug.o
	$(CXX) $(FLAGS) $^ -o $@ -I $(RUNTIME_DIR) -g -O0


clean:
	@rm -rf *.out *.o *.dSYM
	@rm -f $(BIN) $(BIN)-debug 
