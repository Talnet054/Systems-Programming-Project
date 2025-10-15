/* symbol_table.h */
/**
 * @file symbol_table.h
 * @brief This header declares functions for manipulating the symbol table.
 *
 * It defines the interface for adding, finding, and freeing symbols,
 * as well as specific functions for updating data symbol addresses
 * and adding external symbol usages.
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "assembler.h" /* Includes Symbol struct, SymbolType, etc. */
#include <stdio.h>     /* For fprintf, stderr (needed in .c) */

/* --- Function Prototypes --- */

/**
 * @brief Adds a new symbol to the symbol table.
 * Handles checks for reserved keywords, invalid label syntax, and duplicate definitions,
 * including specific rules for EXTERNAL and ENTRY symbols.
 * @param head Pointer to the head of the Symbol linked list (will be updated if new symbol is added at head).
 * @param name The name of the symbol.
 * @param address The memory address associated with the symbol.
 * @param type The type of the symbol (CODE, DATA, EXTERNAL, ENTRY).
 * @param line_num The line number in the source file where the symbol is defined/declared (for error reporting).
 */
void addSymbol(Symbol **head, const char* name, int address, SymbolType type, int line_num);

/**
 * @brief Searches for a symbol by name in the symbol table.
 * @param head Pointer to the head of the Symbol linked list.
 * @param name The name of the symbol to find.
 * @return A pointer to the Symbol structure if found, otherwise NULL.
 */
Symbol* findSymbol(Symbol* head, const char* name);

/**
 * @brief Frees the entire symbol table, including all Symbol structures
 * and any associated external usage lists.
 * @param head Pointer to the head of the Symbol linked list.
 */
void freeSymbolTable(Symbol* head);

/**
 * @brief Updates the addresses of SYMBOL_DATA entries by adding the final
 * Instruction Counter (ICF) value. This is done at the end of the first pass.
 * @param head Pointer to the head of the Symbol linked list.
 * @param icf The final Instruction Counter value from the first pass.
 */
void updateDataSymbolsAddresses(Symbol *head, int icf);

/**
 * @brief Adds a usage address for an external symbol.
 * Called during the second pass when an external symbol is referenced.
 * The address is added to the `external_usages` linked list within the Symbol structure.
 * @param sym A pointer to the Symbol structure (must be of type SYMBOL_EXTERNAL).
 * @param address The memory address (IC value) where the external symbol is referenced.
 */
void addExternalUsage(Symbol *sym, int address);

/**
 * @brief Prints the contents of the symbol table to stdout for debugging purposes.
 * @param head Pointer to the head of the Symbol linked list.
 */
void printSymbolTable(Symbol* head);



#endif