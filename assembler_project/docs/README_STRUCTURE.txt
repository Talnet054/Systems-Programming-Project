PROJECT STRUCTURE
=================

assembler_organized/
│
├── assembler          # Executable (compiled binary)
├── Makefile          # Build configuration
├── README_STRUCTURE.txt
│
├── src/              # Source files (.c)
│   ├── main.c
│   ├── first_pass.c
│   ├── second_pass.c
│   ├── symbol_table.c
│   ├── output_files.c
│   ├── convertToBase4.c
│   └── macro_processor.c
│
├── include/          # Header files (.h)
│   ├── assembler.h
│   ├── first_pass.h
│   ├── second_pass.h
│   ├── symbol_table.h
│   ├── output_files.h
│   ├── convertToBase4.h
│   ├── macro_processor.h
│
├── tests/            # Test files (.as)
│   ├── ps.as
│   ├── ps_fixed.as
│   ├── test_all_instructions.as
│   ├── test_addressing_modes.as
│   ├── test_are_encoding.as
│   ├── test_boundaries.as
│   ├── test_complex.as
│   ├── test_edge_cases.as
│   ├── test_errors.as
│   ├── test_symbols.as
│   ├── test_table_instructions.as
│   ├── test_mov.as
│   ├── pdf_test.as
│   ├── pdf_macro_test.as
│   ├── pdf_data_test.as
│   ├── simple_test.as
│   └── debug_ps.as
│
└── docs/             # Documentation
    ├── README.txt
    └── QUICK_START.txt

TO COMPILE:
-----------
make clean
make

TO RUN:
-------
./assembler tests/ps

