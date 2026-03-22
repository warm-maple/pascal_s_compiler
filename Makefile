# Pascal-S 编译器 Makefile

CXX = g++
BISON = bison
FLEX = flex

CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Isrc -Ibuild

SRC_DIR = src
BUILD_DIR = build

PARSER_CPP = $(BUILD_DIR)/parser.tab.cpp
PARSER_HPP = $(BUILD_DIR)/parser.tab.hpp
LEXER_CPP = $(BUILD_DIR)/lexer.cpp

SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/ast.cpp
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/ast.o $(BUILD_DIR)/parser.tab.o $(BUILD_DIR)/lexer.o

TARGET = pascal-s-compiler

TEST_DIR = open_set
OUTPUT_DIR = test_output

.PHONY: all clean test dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OUTPUT_DIR)

$(PARSER_CPP) $(PARSER_HPP): $(SRC_DIR)/parser.y
	$(BISON) -d -o $(PARSER_CPP) $(SRC_DIR)/parser.y

$(LEXER_CPP): $(SRC_DIR)/lexer.l $(PARSER_HPP)
	$(FLEX) -o $(LEXER_CPP) $(SRC_DIR)/lexer.l

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(PARSER_HPP) src/codegen.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.cpp $(SRC_DIR)/ast.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_DIR)/ast.cpp -o $(BUILD_DIR)/ast.o

$(BUILD_DIR)/parser.tab.o: $(PARSER_CPP)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(PARSER_CPP) -o $(BUILD_DIR)/parser.tab.o

$(BUILD_DIR)/lexer.o: $(LEXER_CPP)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(LEXER_CPP) -o $(BUILD_DIR)/lexer.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(OUTPUT_DIR)

test: $(TARGET)
	@echo "Running tests..."
	@passed=0; failed=0; \
	for test_file in $(TEST_DIR)/0*.pas; do \
		test_name=$$(basename "$$test_file" .pas); \
		echo -n "Testing $$test_name... "; \
		if ./$(TARGET) "$$test_file" -o $(OUTPUT_DIR)/"$${test_name}.c" 2>/dev/null; then \
			echo "OK"; \
			passed=$$((passed + 1)); \
		else \
			echo "FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$passed passed, $$failed failed"

test-single: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make test-single FILE=<test.pas>"; \
		exit 1; \
	fi
	./$(TARGET) $(FILE) -o $(OUTPUT_DIR)/output.c
	@echo "Generated C code:"
	@cat $(OUTPUT_DIR)/output.c

run-test: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run-test FILE=<test.pas>"; \
		exit 1; \
	fi
	./$(TARGET) $(FILE) -o $(OUTPUT_DIR)/output.c
	gcc -std=c99 $(OUTPUT_DIR)/output.c -o $(OUTPUT_DIR)/output
	./$(OUTPUT_DIR)/output

debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -DDEBUG
debug: clean all

help:
	@echo "Pascal-S Compiler Makefile"
	@echo "Targets: all, clean, test, test-single FILE=<test.pas>, run-test FILE=<test.pas>, debug, help"
