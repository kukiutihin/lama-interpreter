CXX = clang++
CC = clang
FLAGS = -std=gnu++23 -O2

BIN = app
RUNTIME_DIR = Lama/runtime
SRC = src

all: $(BIN)

runtime.o: $(RUNTIME_DIR)/runtime.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

gc.o: $(RUNTIME_DIR)/gc.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

runtime-debug.o: $(RUNTIME_DIR)/runtime.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value -g

gc-debug.o: $(RUNTIME_DIR)/gc.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value -g

$(BIN): $(SRC)/main.cpp $(SRC)/virtual-machine.cpp runtime.o gc.o
	$(CXX) $(FLAGS) $^ -o $@ -I $(RUNTIME_DIR) 

$(BIN)-debug: $(SRC)/main.cpp $(SRC)/virtual-machine.cpp runtime-debug.o gc-debug.o
	$(CXX) $(FLAGS) $^ -o $@ -I $(RUNTIME_DIR) -g

clean:
	@rm -f *.out *.o
	@rm -f $(BIN) $(BIN)-debug