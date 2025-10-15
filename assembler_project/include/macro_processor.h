/* macro_processor.h */
/**
 * @file macro_processor.h
 * @brief Declares the interface for the assembler's macro processing module.
 *
 * Defines the Macro structure and prototypes for functions that handle
 * macro definitions, expansion, and memory management.
 */

#ifndef MACRO_PROCESSOR_H
#define MACRO_PROCESSOR_H

#include "assembler.h" /* Include common definitions like Macro struct, MAX_SYMBOL_LENGTH */

/**
 * @brief Reads macro definitions from the input file and stores them in a linked list.
 * Performs basic syntax checks for macro definitions.
 * @param input The input file pointer.
 * @return A pointer to the head of the Macro linked list, or NULL if no macros were found/processed.
 */
Macro* processMacroDefinitions(FILE* input);
char* expandMacroInLine(const char* line, Macro* macroList);

/**
 * @brief Expands a single line of input. If the line is a macro call, it returns the expanded content.
 * Otherwise, it returns a duplicate of the original line.
 * The returned string is dynamically allocated and must be freed by the caller.
 * @param line The input line to expand.
 * @param macroList A pointer to the head of the Macro linked list.
 * @return A dynamically allocated string containing the expanded line(s), or a duplicate of the original line.
 * The returned string must be freed by the caller.
 */
char* expandMacros(const char* line, Macro* macroList);

/**
 * @brief Frees all dynamically allocated memory for the Macro linked list and its contents.
 * @param head A pointer to the head of the Macro linked list.
 */
void freeMacroList(Macro* head);

#endif