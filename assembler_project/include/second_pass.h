/* second_pass.h */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "assembler.h" /* Includes Instruction and Symbol structs */

/**
 * Performs the second pass of the assembler.
 * Iterates through the instruction list, resolves symbol references,
 * generates final machine code, and collects external symbol usages.
 * @param instructionList A pointer to the head of the Instruction linked list (populated in first pass).
 * @param symTab A pointer to the head of the Symbol linked list (finalized in first pass).
 * @return 1 if the second pass completed successfully, 0 otherwise (if g_has_error is set).
 */
int secondPass(Instruction *instructionList, Symbol *symTab);

#endif