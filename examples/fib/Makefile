CC = clang
CFLAGS = -Wall -O0
LLVM_FLAGS = -S -emit-llvm -O0

SRC = fib.c
TARGET = fib
LLVM_IR = fib.ll

all: $(TARGET) $(LLVM_IR)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(LLVM_IR): $(SRC)
	$(CC) $(LLVM_FLAGS) -o $(LLVM_IR) $(SRC)

ir: $(LLVM_IR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(LLVM_IR)

.PHONY: all ir run clean
