CC = clang
CFLAGS = -Wall -O0
LLVM_FLAGS = -S -emit-llvm -O0

SRC = bank_flow.c
TARGET = bank_flow
LLVM_IR = bank_flow.ll
CONFIG = bank_flow_config.toml
ANALYZER = ../../analyzer

all: $(TARGET) $(LLVM_IR)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(LLVM_IR): $(SRC)
	$(CC) $(LLVM_FLAGS) -o $(LLVM_IR) $(SRC)

analyze: $(LLVM_IR)
	$(ANALYZER) --config $(CONFIG) $(LLVM_IR)

ir: $(LLVM_IR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(LLVM_IR)

.PHONY: all analyze ir run clean
