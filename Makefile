CXX = g++
CXXFLAGS = -Wall -g -std=c++17
TARGET = analyzer
SRCS = main.cpp CFG.cpp function.cpp basic_block.cpp nodes.cpp security_policy.cpp
OBJS = $(SRCS:.cpp=.o)
TESTS = $(wildcard test_suite/*.ll)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

test: $(TARGET)
	@fail=0; \
	for test in $(TESTS); do \
		printf "Running %s... " "$$test"; \
		output=$$(./$(TARGET) "$$test" 2>&1); \
		status=$$?; \
		if [ $$status -ne 0 ]; then \
			echo "FAIL (exit $$status)"; \
			echo "$$output"; \
			fail=1; \
		elif printf "%s\n" "$$output" | grep -q "Unsupported LLVM instruction"; then \
			echo "FAIL (unsupported instruction)"; \
			echo "$$output"; \
			fail=1; \
		else \
			echo "PASS"; \
		fi; \
	done; \
	exit $$fail

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all test clean
