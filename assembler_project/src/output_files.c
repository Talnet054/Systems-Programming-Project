/* output_files.c */
/**
 * @file output_files.c
 * @brief This module is responsible for writing the assembler's output files.
 * 
 * Generates three types of output files:
 * 1. Object file (.ob) - Machine code in base-4 format
 * 2. Entries file (.ent) - List of entry points and their addresses
 * 3. Externals file (.ext) - List of external symbol usage locations
 */

#include "output_files.h"
#include "assembler.h"
#include "convertToBase4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_FILENAME_LENGTH
#define MAX_FILENAME_LENGTH 256
#endif

/* --- External dependency: global error flag --- */
extern int g_has_error;

/**
 * Writes the main object file containing all machine code
 * Format:
 *   Line 1: IC DC (instruction count, data count in base-4)
 *   Following lines: address<TAB>machine_code (both in base-4)
 * 
 * The file contains:
 * - All instruction words with their addresses
 * - All data values with their addresses (after instructions)
 * 
 * @param filename Base filename (without extension)
 * @param ICF Instruction Code Final - total instruction words used
 * @param DCF Data Counter Final - total data words used
 * @param dataList Linked list of data items to write
 * @param instructionList Linked list of instructions to write
 */
void writeObjectFile(const char *filename, int ICF, int DCF, DataItem *dataList, Instruction *instructionList) {
    /* All variables declared at top for C90 compliance */
    FILE *file;
    char obj_filename[MAX_FILENAME_LENGTH];
    char *base4_address;
    char *base4_value;
    char *base4_icf;
    char *base4_dcf;
    char *stripped_icf;  /* For removing leading 'a's from header */
    char *stripped_dcf;  /* For removing leading 'a's from header */
    Instruction *inst;
    DataItem *data;
    int i;

    /* Create output filename */
    sprintf(obj_filename, "%s.ob", filename);
    
    /* Open file for writing */
    file = fopen(obj_filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create object file '%s'.\n", obj_filename);
        g_has_error = 1;
        return;
    }

    /* --- Part 1: Write header line --- */
    /* Header contains instruction count and data count in base-4 */
    
    /* Convert counts to base-4 */
    base4_icf = convertToBase4(ICF);
    base4_dcf = convertToBase4(DCF);
    if (!base4_icf || !base4_dcf) {
        fprintf(stderr, "Error: Memory allocation failed for header.\n");
        g_has_error = 1; 
        fclose(file); 
        return;
    }
    
    /* Strip leading 'a's (zeros) for cleaner output format */
    /* This matches the expected format in the course PDF */
    stripped_icf = stripLeadingA(base4_icf);
    stripped_dcf = stripLeadingA(base4_dcf);
    
    if (!stripped_icf || !stripped_dcf) {
        fprintf(stderr, "Error: Memory allocation failed for stripped header.\n");
        free(base4_icf);
        free(base4_dcf);
        g_has_error = 1; 
        fclose(file); 
        return;
    }
    
    /* Write the header line */
    fprintf(file, "%s %s\n", stripped_icf, stripped_dcf);
    
    /* Clean up header memory */
    free(base4_icf);
    free(base4_dcf);
    free(stripped_icf);
    free(stripped_dcf);

    /* --- Part 2: Write all instructions --- */
    /* Each instruction may span multiple words (1-5 words) */
    
    inst = instructionList;
    while (inst) {
        /* Write main instruction word */
        base4_address = convertToBase4(inst->address);
        if (!base4_address) {
            fprintf(stderr, "Error: Memory allocation failed for instruction address.\n");
            g_has_error = 1; 
            fclose(file); 
            return;
        }
        
        /* Address and machine code separated by tab */
        fprintf(file, "%s\t%s\n", base4_address, inst->machine_code_base4);
        free(base4_address);

        /* Write additional operand words if present */
        /* These follow immediately after the main instruction */
        for (i = 0; i < inst->num_operand_words; i++) {
            /* Calculate address for this operand word */
            base4_address = convertToBase4(inst->address + i + 1);
            if (!base4_address) {
                fprintf(stderr, "Error: Memory allocation failed for operand word address.\n");
                g_has_error = 1; 
                fclose(file); 
                return;
            }
            
            /* Write operand word */
            fprintf(file, "%s\t%s\n", base4_address, inst->operand_words_base4[i]);
            free(base4_address);
        }
        
        /* Move to next instruction */
        inst = inst->next;
    }

    /* --- Part 3: Write all data values --- */
    /* Data section comes after all instructions in memory */
    
    data = dataList;
    while (data) {
        /* Calculate actual memory address for data */
        /* Data starts at: MEMORY_START + ICF (after all instructions) */
        base4_address = convertToBase4(data->address + ICF + MEMORY_START);
        if (!base4_address) {
            fprintf(stderr, "Error: Memory allocation failed for data item address.\n");
            g_has_error = 1; 
            fclose(file); 
            return;
        }
        
        /* Convert data value to base-4 */
        base4_value = convertToBase4(data->value);
        if (!base4_value) {
            fprintf(stderr, "Error: Memory allocation failed for data value.\n");
            free(base4_address); 
            g_has_error = 1; 
            fclose(file); 
            return;
        }
        
        /* Write data word */
        fprintf(file, "%s\t%s\n", base4_address, base4_value);
        
        /* Clean up */
        free(base4_address);
        free(base4_value);
        
        /* Move to next data item */
        data = data->next;
    }

    fclose(file);
    printf("Generated object file: %s\n", obj_filename);
}

/**
 * Writes the entries file listing all entry points
 * Entry points are symbols marked with .entry directive
 * Format: symbol_name address (in base-4)
 * 
 * Only generated if at least one valid entry exists
 * 
 * @param filename Base filename (without extension)
 * @param symTab Symbol table containing all symbols
 */
void writeEntriesFile(const char *filename, Symbol *symTab) {
    /* All variables declared at top for C90 compliance */
    FILE *file = NULL;
    char ent_filename[MAX_FILENAME_LENGTH];
    Symbol *symbol = symTab;
    int entry_found = 0;
    char *base4_address;

    /* First pass: check if any entry symbols exist */
    /* Entry must have valid address (>= MEMORY_START) */
    while (symbol) {
        if (symbol->type == SYMBOL_ENTRY && symbol->address >= MEMORY_START) {
            entry_found = 1;
            break;
        }
        symbol = symbol->next;
    }

    /* Don't create file if no entries */
    if (!entry_found) {
        printf("No valid entry symbols found. '%s.ent' will not be generated.\n", filename);
        return;
    }

    /* Create entries file */
    sprintf(ent_filename, "%s.ent", filename);
    file = fopen(ent_filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create entries file '%s'.\n", ent_filename);
        g_has_error = 1;
        return;
    }

    /* Second pass: write all entry symbols */
    symbol = symTab;
    while (symbol) {
        if (symbol->type == SYMBOL_ENTRY && symbol->address >= MEMORY_START) {
            /* Convert address to base-4 */
            base4_address = convertToBase4(symbol->address);
            if (!base4_address) {
                fprintf(stderr, "Error: Memory allocation failed for entry address.\n");
                g_has_error = 1; 
                fclose(file); 
                return;
            }
            
            /* Write entry: name and address */
            fprintf(file, "%s %s\n", symbol->name, base4_address);
            free(base4_address);
        }
        symbol = symbol->next;
    }

    fclose(file);
    printf("Generated entries file: %s\n", ent_filename);
}

/**
 * Writes the externals file listing all external symbol usages
 * External symbols are defined in other files and imported with .extern
 * Format: symbol_name address (for each usage location)
 * 
 * Only generated if at least one external is actually used
 * 
 * @param filename Base filename (without extension)
 * @param symTab Symbol table containing all symbols and their usage lists
 */
void writeExternalsFile(const char *filename, Symbol *symTab) {
    /* All variables declared at top for C90 compliance */
    FILE *file = NULL;
    char ext_filename[MAX_FILENAME_LENGTH];
    Symbol *symbol = symTab;
    int external_usage_found = 0;
    ExternalUsage *current;
    char *base4_address;

    /* First pass: check if any external symbols are actually used */
    /* Declaration alone isn't enough - must be referenced */
    while (symbol) {
        if (symbol->type == SYMBOL_EXTERNAL && symbol->external_usages != NULL) {
            external_usage_found = 1;
            break;
        }
        symbol = symbol->next;
    }

    /* Don't create file if no externals are used */
    if (!external_usage_found) {
        printf("No external symbol usages found. '%s.ext' will not be generated.\n", filename);
        return;
    }

    /* Create externals file */
    sprintf(ext_filename, "%s.ext", filename);
    file = fopen(ext_filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create externals file '%s'.\n", ext_filename);
        g_has_error = 1;
        return;
    }

    /* Second pass: write all external symbol usages */
    symbol = symTab;
    while (symbol) {
        if (symbol->type == SYMBOL_EXTERNAL) {
            /* Walk through all usage locations for this external */
            current = symbol->external_usages;
            while (current) {
                /* Convert usage address to base-4 */
                base4_address = convertToBase4(current->address);
                if (!base4_address) {
                    fprintf(stderr, "Error: Memory allocation failed for external usage address.\n");
                    g_has_error = 1; 
                    fclose(file); 
                    return;
                }
                
                /* Write one line per usage location */
                fprintf(file, "%s %s\n", symbol->name, base4_address);
                free(base4_address);
                
                /* Move to next usage */
                current = current->next;
            }
        }
        symbol = symbol->next;
    }
    
    fclose(file);
    printf("Generated externals file: %s\n", ext_filename);
}