/* assembler.h */
/**
 * @file assembler.h
 * @brief This header defines necessary constants, global data types, and core data structures
 * for the assembler project.
 *
 * It includes definitions for memory organization, symbol table entries,
 * instruction and data representations, and macro definitions.
 * It also declares the global error flag and prototypes for common helper functions
 * used across multiple modules.
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* Required for string manipulation functions */
#include <ctype.h>  /* Required for isspace, isalpha, isdigit, etc. */

/* --- Constants for Assembler Configuration --- */
#define MEMORY_START 100            /**< Starting memory address for instructions. */
#define MAX_LINE_LENGTH 81          /**< Maximum characters per source line (80 + '\n' + '\0'). */
#define MAX_SYMBOL_LENGTH 31        /**< Maximum length for a symbol name (30 chars + '\0'). */
#define MAX_OPCODE_LENGTH 5         /**< Max opcode string length (e.g., "stop" + '\0'). */
#define MAX_REGISTER_NAME_LENGTH 3  /**< Max register name length (e.g., "r7" + '\0'). */
#define BASE4_WORD_LENGTH 5         /**< Length of a machine word in base-4 representation (10 bits = 5 base-4 digits). */

#define INITIAL_MACRO_LINES_CAPACITY 10 /**< Initial capacity for dynamic array storing macro lines. */

/* --- A,R,E Bit Encoding Constants --- */
/**
 * A,R,E (Absolute/Relocatable/External) bits are the least significant 2 bits
 * of each instruction word, indicating how the address should be handled:
 * - 00 (A): Absolute - no relocation needed (immediates, registers)
 * - 01 (E): External - address provided by linker
 * - 10 (R): Relocatable - address adjusted based on load location
 * Note: 11 is undefined for instructions but valid for data words.
 */
#define ARE_ABSOLUTE    'a'  /* 00 - Absolute addressing */
#define ARE_EXTERNAL    'b'  /* 01 - External symbol */
#define ARE_RELOCATABLE 'c'  /* 10 - Relocatable address */
/* Note: Data words don't use A,R,E encoding and can use all 10 bits */

/* --- Enum Definitions --- */

/**
 * @brief Defines the types of symbols (labels) that can be stored in the symbol table.
 */
typedef enum {
    SYMBOL_CODE,      /**< Symbol refers to an instruction address. */
    SYMBOL_DATA,      /**< Symbol refers to a data address (from .data, .string, .mat). */
    SYMBOL_EXTERNAL,  /**< Symbol is declared external (.extern). Its address is resolved by linker. */
    SYMBOL_ENTRY      /**< Symbol is declared as an entry point (.entry). */
} SymbolType;

/* --- Structure Forward Declarations --- */
/* Used to resolve circular dependencies between Symbol and ExternalUsage.*/
typedef struct ExternalUsage ExternalUsage;

/* --- Structure Definitions --- */

/**
 * @brief Represents a symbol (label) in the assembler's symbol table.
 * Contains the symbol's name, its memory address, and its type.
 * For external symbols, it also links to a list of places where it's used.
 */
typedef struct Symbol {
    char name[MAX_SYMBOL_LENGTH];   /**< The name of the symbol. */
    int address;                    /**< The memory address of the symbol. */
    SymbolType type;                 /**< The type of the symbol (Code, Data, External, Entry). */
    struct Symbol *next;            /**< Pointer to the next symbol in the linked list. */
    ExternalUsage *external_usages; /**< Head of a linked list tracking where this external symbol is referenced. */
} Symbol;

/**
 * @brief Tracks a single instance where an external symbol is referenced in the code.
 * Used to generate the .ext (externals) output file.
 */
struct ExternalUsage {
    int address;          /**< The memory address (IC value) where the external symbol is referenced. */
    ExternalUsage *next;  /**< Pointer to the next external usage in the list. */
};

/**
 * @brief Represents a single machine instruction parsed from the source code.
 * Stores parsed operands and eventually the encoded machine code.
 */
typedef struct Instruction {
    int address;                                          /**< The instruction's memory address (IC value). */
    int original_line_number;                             /**< The original line number from the source .as file for error reporting. */
    char opcode[MAX_OPCODE_LENGTH];                       /**< The opcode string (e.g., "mov", "add"). */
    int num_operands;                                     /**< Number of operands (0, 1, or 2) parsed for this instruction. */
    char operand1[MAX_LINE_LENGTH];                       /**< String representation of the first operand. */
    char operand2[MAX_LINE_LENGTH];                       /**< String representation of the second operand. */
    int instruction_length;                               /**< The total length of the instruction in machine words (1-5). */
    char machine_code_base4[BASE4_WORD_LENGTH + 1];       /**< The first word of machine code (opcode word) in base-4. */
    char operand_words_base4[4][BASE4_WORD_LENGTH + 1];   /**< Up to 4 additional words for operands in base-4. */
    int num_operand_words;                                /**< The actual number of additional operand words generated. */
    
    struct Instruction* next;                             /**< Pointer to the next instruction in the linked list. */
} Instruction;
/**
 * @brief Represents a data item parsed from .data, .string, or .mat directives.
 * Stores the raw value and its base-4 representation.
 */
typedef struct DataItem {
    int address;                           /**< The data item's memory address (DC value). */
    int value;                             /**< The raw integer value of the data item. */
    char base4_representation[BASE4_WORD_LENGTH + 1]; /**< The base-4 string representation of the value. */
    struct DataItem* next;                 /**< Pointer to the next data item in the linked list. */
} DataItem;

/**
 * @brief Represents a macro definition.
 * Stores the macro's name and its content lines.
 */
typedef struct Macro {
    char name[MAX_SYMBOL_LENGTH];          /**< The name of the macro. */
    char **lines;                          /**< Dynamic array of strings (lines) that make up the macro's content. */
    int lineCount;                         /**< The number of lines currently stored in the macro. */
    int capacity;                          /**< The current allocated capacity for the 'lines' array. */
    struct Macro *next;                    /**< Pointer to the next macro definition in the linked list. */
} Macro;




/* --- Global Variables --- */

/**
 * @brief Global error flag. Set to 1 if any assembly error occurs,
 * preventing the generation of output files.
 */
extern int g_has_error; /* Defined in main.c */

/* --- Common Helper Function Prototypes (implemented in first_pass.c or utilities.c) --- */

/**
 * @brief Checks if the given string is a valid opcode.
 * @param s The string to check.
 * @return 1 if 's' is an opcode, 0 otherwise.
 */
extern int is_opcode(const char* s);

/**
 * @brief Checks if the given string is a valid register name (e.g., "r0" - "r7").
 * @param s The string to check.
 * @return 1 if 's' is a register name, 0 otherwise.
 */
extern int is_register(const char* s);

/**
 * @brief Checks if the given string is a valid label name according to assembler rules.
 * @param s The string to check.
 * @return 1 if 's' is a valid label name, 0 otherwise.
 */
extern int is_valid_label(const char* s);

#endif