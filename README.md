# C Assembler Project

A complete two-pass assembler for a custom assembly language, developed as a final project for the Systems Programming Laboratory course at The Open University.

## Key Features

* **Two-Pass Algorithm:** Implements the classic two-pass mechanism to first build a symbol table and then translate the code.
* **Macro Support:** Includes a pre-assembler stage that handles macro expansion.
* **Error Handling:** Detects and reports a wide range of syntactical errors in the source assembly files.
* **Multiple Output Files:** Generates object, entry, and external files, demonstrating an understanding of the linking process.

## Technologies Used

* **Language:** C (C99)
* **Build System:** Makefile
* **Core Concepts:** Dynamic Data Structures (Linked Lists), Manual Memory Management, File I/O.

## Getting Started

To compile and run the assembler:

1.  **Clone the repository:**
    ```sh
    git clone [https://github.com/Talnet054/Systems-Programming-Project.git](https://github.com/Talnet054/Systems-Programming-Project.git)
    ```
2.  **Navigate to the project directory:**
    ```sh
    cd Systems-Programming-Project
    ```
3.  **Compile the project using the Makefile:**
    ```sh
    make
    ```
4.  **Run the assembler with an assembly file:**
    ```sh
    ./assembler <your_assembly_file_name>
    ```
