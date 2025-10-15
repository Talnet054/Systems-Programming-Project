# =====================================================
#         MAKEFILE FOR ASSEMBLER PROJECT
#     Course: 20465 - Systems Programming Laboratory
# =====================================================
# This Makefile automates the compilation process of our
# two-pass assembler that converts assembly language to
# machine code in special base-4 format.
# =====================================================

# === COMPILER CONFIGURATION ===
# CC: Specifies which C compiler to use (GNU C Compiler)
CC = gcc

# CFLAGS: Compilation flags that control how the compiler behaves
# -Wall: Enable all common warning messages
# -ansi: Enforce ANSI C89/C90 standard (required by course)
# -pedantic: Strictly enforce the C standard, reject non-standard code
# -g: Include debugging information in the executable
# -Iinclude: Tell compiler to look for header files in 'include/' directory
CFLAGS = -Wall -ansi -pedantic -g -Iinclude

# === TARGET CONFIGURATION ===
# TARGET: Name of the final executable program we're building
# This will be the command users type to run the assembler
TARGET = assembler

# === OBJECT FILES ===
# OBJS: List of intermediate object files (.o) that need to be created
# Each .o file corresponds to a .c source file after compilation
OBJS = main.o \
       macro_processor.o \
       first_pass.o \
       second_pass.o \
       symbol_table.o \
       output_files.o \
       convertToBase4.o

# =====================================================
#                    BUILD RULES
# =====================================================

# === DEFAULT TARGET ===
# 'all' is the default target - runs when user types just 'make'
# It depends on $(TARGET), so it will build the assembler executable
all: $(TARGET)

# === LINKING RULE ===
# This rule combines all object files into the final executable
# $(TARGET): The file we're creating (assembler)
# $(OBJS): The dependencies - all object files must exist first
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# =====================================================
#           COMPILATION RULES FOR EACH MODULE
# =====================================================
# Each rule below compiles one .c file into a .o file
# The -c flag tells gcc to compile only (not link)
# The -o flag specifies the output file name

# === MAIN MODULE ===
# Entry point of the program, handles command-line arguments
main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o main.o

# === MACRO PROCESSOR MODULE ===
# Pre-assembler phase: expands macros in the source code
# Creates .am files with expanded macros
macro_processor.o: src/macro_processor.c
	$(CC) $(CFLAGS) -c src/macro_processor.c -o macro_processor.o

# === FIRST PASS MODULE ===
# First pass of assembly: builds symbol table, allocates memory
# Identifies labels and calculates their addresses
first_pass.o: src/first_pass.c
	$(CC) $(CFLAGS) -c src/first_pass.c -o first_pass.o

# === SECOND PASS MODULE ===
# Second pass of assembly: generates actual machine code
# Resolves symbol references and creates binary output
second_pass.o: src/second_pass.c
	$(CC) $(CFLAGS) -c src/second_pass.c -o second_pass.o

# === SYMBOL TABLE MODULE ===
# Manages the symbol table data structure
# Stores and retrieves labels, their addresses, and attributes
symbol_table.o: src/symbol_table.c
	$(CC) $(CFLAGS) -c src/symbol_table.c -o symbol_table.o

# === OUTPUT FILES MODULE ===
# Handles creation of output files (.ob, .ent, .ext)
# Formats the machine code according to specifications
output_files.o: src/output_files.c
	$(CC) $(CFLAGS) -c src/output_files.c -o output_files.o

# === BASE-4 CONVERTER MODULE ===
# Converts binary numbers to the special base-4 format
# Uses 'a', 'b', 'c', 'd' instead of 0, 1, 2, 3
convertToBase4.o: src/convertToBase4.c
	$(CC) $(CFLAGS) -c src/convertToBase4.c -o convertToBase4.o

# =====================================================
#                  UTILITY TARGETS
# =====================================================

# === CLEAN TARGET ===
# Removes all compiled files to force a fresh rebuild
# Usage: make clean
clean:
	rm -f $(OBJS) $(TARGET)

# === PHONY TARGETS ===
# .PHONY tells Make that these targets don't create actual files
# This prevents conflicts if files named 'all' or 'clean' exist
.PHONY: all clean

# =====================================================
#                   USAGE INSTRUCTIONS
# =====================================================
# To compile the assembler:        make
# To remove compiled files:        make clean
# To rebuild from scratch:         make clean && make
# To run the assembler:            ./assembler <filename>
# =====================================================
