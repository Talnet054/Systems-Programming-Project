/**
 * @file symbol_table.c
 * @brief Implements functions for managing the assembler's symbol table.
 *
 * This module handles adding, finding, and freeing symbols,
 * including complex logic for duplicate symbol checks, reserved word validation,
 * and management of external symbol usages.
 */

#include "symbol_table.h"
#include <stdio.h>  /* For fprintf, stderr */
#include <stdlib.h> /* For malloc, free, exit */
#include <string.h> /* For strcmp, strncpy */

/* --- External Dependencies (from other modules) --- */
/* These are declared in assembler.h and implemented elsewhere (e.g., first_pass.c) */
extern int g_has_error;
extern int is_opcode(const char* s);
extern int is_register(const char* s);
extern int is_valid_label(const char* s);


/**
 * @brief Searches for a symbol by name in the symbol table.
 * @param head Pointer to the head of the Symbol linked list.
 * @param name The name of the symbol to find.
 * @return A pointer to the Symbol structure if found, otherwise NULL.
 */
Symbol* findSymbol(Symbol* head, const char* name) {
    Symbol *current = head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

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
void addSymbol(Symbol **head, const char *name, int address, SymbolType type, int line_num) {

    Symbol *existingSymbol;
    Symbol *newSymbol;

    /* 1. Check if name is a reserved keyword (opcode or register) */
    if (is_opcode(name) || is_register(name)) {
        fprintf(stderr, "Error at line %d: Symbol '%s' is a reserved keyword and cannot be used as a label.\n", line_num, name);
        g_has_error = 1;
        return;
    }

    /* 2. Check if label syntax is valid (starts with alpha, then alphanumeric) */
    if (!is_valid_label(name)) {
        fprintf(stderr, "Error at line %d: Invalid label name '%s'. Labels must start with an alphabetic character and contain only alphanumeric characters, max %d chars.\n", line_num, name, MAX_SYMBOL_LENGTH - 1);
        g_has_error = 1;
        return;
    }

    /* 3. Check if symbol already exists in the table */
    existingSymbol = findSymbol(*head, name);
    if (existingSymbol != NULL) {
        /* Handle cases for already existing symbol based on types */
        if (type == SYMBOL_EXTERNAL) {
            /* If new type is EXTERNAL: */
            /* - If existing is CODE/DATA: Error (cannot be internal and external) */
            if (existingSymbol->type == SYMBOL_CODE || existingSymbol->type == SYMBOL_DATA) {
                fprintf(stderr, "Error at line %d: Symbol '%s' is defined internally and declared as external.\n", line_num, name);
                g_has_error = 1;
                return;
            }
            /* - If existing is SYMBOL_EXTERNAL: Redundant declaration, ignore. */
            /* - If existing is SYMBOL_ENTRY: Error (cannot be entry and external simultaneously) */
            if (existingSymbol->type == SYMBOL_ENTRY) {
                 fprintf(stderr, "Error at line %d: Symbol '%s' is declared as .entry and .extern (mutually exclusive).\n", line_num, name);
                 g_has_error = 1;
                 return;
            }
            /* If existing is already EXTERNAL, it's a valid re-declaration, so do nothing. */
            return;
        } else if (type == SYMBOL_ENTRY) {
            /* If new type is ENTRY: */
            /* - If existing is EXTERNAL: Error (cannot be entry and external simultaneously) */
            if (existingSymbol->type == SYMBOL_EXTERNAL) {
                fprintf(stderr, "Error at line %d: Symbol '%s' is declared as .entry and .extern (mutually exclusive).\n", line_num, name);
                g_has_error = 1;
                return;
            }
            /* - If existing is CODE/DATA or already ENTRY: Mark as entry point. This is fine. */
            existingSymbol->type = SYMBOL_ENTRY;
            return;
        } else { /* New type is CODE or DATA (internal definition) */
            /* If existing is CODE/DATA: Duplicate definition error */
            if (existingSymbol->type == SYMBOL_CODE || existingSymbol->type == SYMBOL_DATA) {
                fprintf(stderr, "Error at line %d: Symbol '%s' is already defined internally (CODE/DATA).\n", line_num, name);
                g_has_error = 1;
                return;
            }
            /* If existing is EXTERNAL: It's now defined internally, which is an error. */
            if (existingSymbol->type == SYMBOL_EXTERNAL) {
                fprintf(stderr, "Error at line %d: Symbol '%s' was declared external but is now defined locally.\n", line_num, name);
                g_has_error = 1;
                return;
            }
            /* If existing is ENTRY (but not yet defined as CODE/DATA): Now it is defined. */
            if (existingSymbol->type == SYMBOL_ENTRY) {
                if (existingSymbol->address == 0) { /* Placeholder entry */
                    existingSymbol->address = address;
                    /* Type remains SYMBOL_ENTRY */
                } else {
                    /* Symbol was already defined, and now defined again. This is a duplicate definition error. */
                    fprintf(stderr, "Error at line %d: Symbol '%s' is already defined internally and being redefined.\n", line_num, name);
                    g_has_error = 1;
                }
                return; /* Return after handling the entry symbol update */
            }
        }
    }

    /* If we reach here, the symbol does not exist and we can create it */
    newSymbol = (Symbol *)malloc(sizeof(Symbol));
    if (!newSymbol) {
        fprintf(stderr, "Memory allocation error for symbol '%s'.\n", name);
        exit(1); /* Critical error, terminate program */
    }
    strncpy(newSymbol->name, name, MAX_SYMBOL_LENGTH - 1);
    newSymbol->name[MAX_SYMBOL_LENGTH - 1] = '\0'; /* Ensure null-termination */
    newSymbol->address = address;
    newSymbol->type = type;
    newSymbol->next = *head;
    newSymbol->external_usages = NULL; /* Initialize external usages list to NULL */
    *head = newSymbol;
}

/**
 * @brief Helper function to find a symbol (renamed from 'findSymbol' for clarity within module).
 * @param head Pointer to the head of the Symbol linked list.
 * @param name The name of the symbol to find.
 * @return A pointer to the Symbol structure if found, otherwise NULL.
 */
Symbol* getSymbol(Symbol *head, const char *name) {
    return findSymbol(head, name);
}


/**
 * @brief Frees the entire symbol table, including all Symbol structures
 * and any associated external usage lists.
 * @param head Pointer to the head of the Symbol linked list.
 */
void freeSymbolTable(Symbol *head) {
    
    Symbol *current = head;
    Symbol *next_sym;
    ExternalUsage *current_usage;
    ExternalUsage *next_usage;

    while (current) {
        next_sym = current->next;
        
        /* Free external usages list if it exists */
        if (current->external_usages) {
            current_usage = current->external_usages;
            while(current_usage) {
                next_usage = current_usage->next;
                free(current_usage);
                current_usage = next_usage;
            }
        }

        free(current); /* Free the Symbol struct itself */
        current = next_sym;
    }
}

/**
 * @brief Updates the addresses of SYMBOL_DATA entries by adding the final
 * Instruction Counter (ICF) value. This is done at the end of the first pass.
 * @param head Pointer to the head of the Symbol linked list.
 * @param icf The final Instruction Counter value from the first pass.
 */
void updateDataSymbolsAddresses(Symbol *head, int icf) {
    Symbol *current = head;
    while (current) {
        if (current->type == SYMBOL_DATA) {
             current->address += icf;
        }
        else if (current->type == SYMBOL_ENTRY && current->address < MEMORY_START) {
            current->address += icf;
        }
        current = current->next;
    }
}

/**
 * @brief Adds a usage address for an external symbol.
 * Called during the second pass when an external symbol is referenced.
 * @param sym A pointer to the Symbol structure (must be of type SYMBOL_EXTERNAL).
 * @param address The memory address (IC value) where the external symbol is referenced.
 */
void addExternalUsage(Symbol *sym, int address) {
    
    ExternalUsage *newUsage;

    /* Ensure the symbol is indeed of type EXTERNAL. */
    if (!sym || sym->type != SYMBOL_EXTERNAL) {
        fprintf(stderr, "Internal Error: Attempted to add external usage to a non-external or NULL symbol.\n");
        g_has_error = 1; /* Mark a global error */
        return;
    }

    newUsage = (ExternalUsage *)malloc(sizeof(ExternalUsage));
    if (!newUsage) {
        fprintf(stderr, "Memory allocation error for external usage.\n");
        exit(1); /* Critical memory error, terminate program */
    }
    newUsage->address = address;
    newUsage->next = sym->external_usages; /* Add to head of usages list */
    sym->external_usages = newUsage;
}

/**
 * @brief Prints the contents of the symbol table to stdout for debugging purposes.
 * @param head Pointer to the head of the Symbol linked list.
 */
void printSymbolTable(Symbol* head) {
    
    ExternalUsage *usage;

    printf("====== Symbol Table ======\n");
    while (head) {
        printf("Name: %-10s, Address: %-5d, Type: ", head->name, head->address);
        switch(head->type) {
            case SYMBOL_CODE: printf("CODE"); break;
            case SYMBOL_DATA: printf("DATA"); break;
            case SYMBOL_EXTERNAL: printf("EXTERNAL"); break;
            case SYMBOL_ENTRY: printf("ENTRY"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (head->type == SYMBOL_EXTERNAL && head->external_usages) {
            usage = head->external_usages;
            printf("  Usages: ");
            while(usage) {
                printf("%d ", usage->address);
                usage = usage->next;
            }
            printf("\n");
        }
        head = head->next;
    }
    printf("==========================\n");
}