CXX = clang++
CC = clang
FLAGS = -std=c++23 -O2

BIN = ../app
RUNTIME_DIR ?= ../Lama/runtime

all: $(BIN)

runtime.o: $(RUNTIME_DIR)/runtime.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

gc.o: $(RUNTIME_DIR)/gc.c
	$(CC) $^ -c -o $@ -Wno-shift-negative-value

$(BIN): main.cpp runtime.o gc.o
	$(CXX) $(FLAGS) $^ -o $@ -I $(RUNTIME_DIR) 

clean:
	@rm -f $(BIN)