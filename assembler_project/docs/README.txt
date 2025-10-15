================================================================================
                        ASSEMBLER PROJECT - README
                   Course 20465 - Systems Programming Laboratory
================================================================================

STUDENT INFORMATION:
--------------------
Name: Netanel Tal
ID: 206183139
Submission Date: August 12, 2025

PROJECT DESCRIPTION:
--------------------
Two-pass assembler implementation for a custom assembly language.
Converts assembly code to machine code in base-4 format.

COMPILATION:
------------
To compile the project:
    make clean
    make

Or directly:
    gcc -Wall -ansi -pedantic -g *.c -o assembler

EXECUTION:
----------
To run the assembler:
    ./assembler filename

(Note: filename should be WITHOUT the .as extension)

If the input file is inside the tests/ folder (as in this project),
run the assembler from inside the assembler_organized directory,
including the tests/ path before the filename.

Examples:
    ./assembler tests/ps
    ./assembler tests/simple_test
    ./assembler tests/test_mov

This will process the .as file and generate:
- .ob  : Object file (machine code in base-4)
- .ent : Entry points (if any)
- .ext : External references (if any)
- .am  : Macro-expanded source (if macros exist)

TEST FILES INCLUDED:
--------------------
Main test:
- ps.as : Comprehensive test from course specification

Additional tests:
- test_all_instructions.as : Tests all 16 opcodes
- test_addressing_modes.as : Tests all 4 addressing modes
- test_errors.as : Tests error detection
- test_boundaries.as : Tests edge cases
- pdf_test.as : Test based on PDF example

OUTPUT FORMAT:
--------------
Object files (.ob) use base-4 encoding:
- a=0, b=1, c=2, d=3
- First line: IC (instruction count) DC (data count)
- Following lines: address code (tab-separated)

FEATURES IMPLEMENTED:
---------------------
✓ Two-pass assembly algorithm
✓ Macro processor with expansion
✓ Symbol table management
✓ 16 opcodes (mov, cmp, add, sub, not, clr, lea, inc, dec, jmp, bne, red, prn, jsr, rts, stop)
✓ 4 addressing modes (immediate #, direct, matrix, register)
✓ Error detection and reporting with line numbers
✓ Entry and external symbol handling
✓ Data directives (.data, .string, .mat, .entry, .extern)

COMPILATION FLAGS:
------------------
The project compiles cleanly with:
-Wall -ansi -pedantic

No warnings or errors.

TESTED ON:
----------
- Ubuntu Linux (WSL)
- gcc compiler

NOTES:
------
- Maximum line length: 80 characters
- Immediate values range: -512 to 511
- Starting memory address: 100 (decimal)
- All outputs in base-4 format

================================================================================
