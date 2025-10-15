/* first_pass.h */
/**
 * @file first_pass.h
 * @brief Declares the interface for the assembler's first pass module.
 *
 * Defines the main first pass function and prototypes for helper functions
 * used during the first pass, such as instruction length calculation and operand checks.
 */

#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "assembler.h" /* Includes common definitions like Symbol, Instruction, DataItem */
#include "convertToBase4.h" /* CRITICAL: Must be included for convertToBase4 function declaration */

/* --- Function Prototypes --- */

/**
 * @brief Performs the first pass of the assembler.
 * Reads the assembly source file line by line, builds the symbol table,
 * and populates the instruction and data lists.
 * @param input The file pointer to the assembly source.
 * @param symTab Pointer to the head of the Symbol linked list.
 * @param instructionList Pointer to the head of the Instruction linked list.
 * @param dataList Pointer to the head of the DataItem linked list.
 * @param final_ic_out Pointer to store the final Instruction Counter value.
 * @param final_dc_out Pointer to store the final Data Counter value.
 * @return 1 if the first pass completed successfully without critical errors, 0 otherwise.
 */
int firstPass(FILE *input, Symbol **symTab, Instruction **instructionList, DataItem **dataList, int *final_ic_out, int *final_dc_out);

/* --- Helper Function Prototypes (implemented in first_pass.c) --- */

/**
 * @brief Skips leading whitespace characters in a string.
 * @param s The input string.
 * @return A pointer to the first non-whitespace character.
 */
extern char* skip_whitespace(char* s);

/**
 * @brief Checks if a string represents a valid integer number.
 * @param s The string to check.
 * @return 1 if 's' is a valid integer, 0 otherwise.
 */
extern int is_valid_number(const char *s);

/**
 * @brief Calculates the length of a machine instruction in memory words.
 * Handles cases including immediate, direct, matrix and register operands.
 * @param opcode The instruction opcode string.
 * @param operand1 The string of the first operand.
 * @param operand2 The string of the second operand (empty string if not present).
 * @return Instruction length in memory words (1-5), or -1 on error (e.g., invalid operand count for opcode).
 */
extern int calculate_instruction_length(const char* opcode, const char* operand1, const char* operand2);

/**
 * @brief Validates operands for an instruction (handles source and destination).
 * This function integrates the specific operand type checks required by the assembler.
 * @param opcode The instruction's opcode.
 * @param operand1_str The string for the first operand.
 * @param operand2_str The string for the second operand.
 * @param num_operands_found The number of operands found in the line (0, 1, or 2).
 * @param line_num The current line number for error reporting.
 * @return 1 on success, 0 on failure (error detected and g_has_error set).
 */
int validate_instruction_operands(const char* opcode, const char* operand1_str, const char* operand2_str, int num_operands_found, int line_num);


/*
 * Note: is_opcode and is_register are declared in assembler.h (used globally)
 * and implemented in first_pass.c.
 */

#endif