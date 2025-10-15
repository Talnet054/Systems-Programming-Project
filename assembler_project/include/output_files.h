/* output_files.h */
#ifndef OUTPUT_FILES_H
#define OUTPUT_FILES_H

#include "assembler.h" /* Includes assembler.h (Symbol, Instruction, DataItem, etc.) */
#include "symbol_table.h" /* For Symbol struct specifically for externals/entries */
/* #include "base4_converter.h" // For convertToBase4 function */

/* Declare the global error flag, defined in main.c*/
extern int g_has_error;

/**
 * Writes the object file (.ob).
 * @param filename The name of the object file to create.
 * @param ICF The final Instruction Counter value (total instruction words).
 * @param DCF The final Data Counter value (total data words).
 * @param dataList A pointer to the head of the DataItem linked list.
 * @param instructionList A pointer to the head of the Instruction linked list.
 */
void writeObjectFile(const char *filename, int ICF, int DCF, DataItem *dataList, Instruction *instructionList);

/**
 * Writes the entries file (.ent).
 * @param filename The name of the entries file to create.
 * @param symTab A pointer to the head of the Symbol linked list.
 */
void writeEntriesFile(const char *filename, Symbol *symTab);

/**
 * Writes the externals file (.ext).
 * @param filename The name of the externals file to create.
 * @param symTab A pointer to the head of the Symbol linked list.
 */
void writeExternalsFile(const char *filename, Symbol *symTab);

#endif