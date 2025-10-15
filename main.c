#define _GNU_SOURCE

/* main.c */
/**
 * This is the main file that integrates all assembler modules.
 * It processes multiple files from the command line, handles macro expansion,
 * runs the first and second passes, and writes the output files according to the project specification.
 * 
 * Key Design Decisions:
 * 1. Output files strip leading zeros for readability (matching PDF examples).
 * 2. A,R,E encoding applies only to instruction words, not data.
 * 3. Data words can use full 10-bit range including patterns ending in '11'.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assembler.h"
#include "symbol_table.h"
#include "macro_processor.h"
#include "first_pass.h"
#include "second_pass.h"
#include "output_files.h"

/* Helper function to skip leading whitespace in a string*/
static const char *skip_whitespace_macro(const char *str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

/* Global error flag definition. This is the only place it is defined.*/
/* Other files use 'extern int g_has_error;' to refer to it.*/
int g_has_error = 0;

int main(int argc, char *argv[]) {
    int i; /* Loop counter for processing multiple files */

    int inside_macro_def = 0; /* Flag to track if we're inside a macro definition */

    Instruction *tempInst;
    DataItem *tempData;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1_basename> <file2_basename> ...\n", argv[0]);
        return 1;
    }

    /* Loop to process each file provided as a command-line argument*/
    for (i = 1; i < argc; i++) {
        char input_file_base_name[252];
        char full_input_file_name[300];
        char am_file_name[300];
        FILE *input_file_stream = NULL;
        FILE *am_file_stream = NULL;
        Macro *macro_list = NULL;
        char line[MAX_LINE_LENGTH + 2];
        char *expanded_line_content;
        Symbol *symbol_table = NULL;
        Instruction *instruction_list = NULL;
        DataItem *data_list = NULL;
        int final_ic = 0;
        int final_dc = 0;

        /* --- Reset all data for the new file ---*/
        strncpy(input_file_base_name, argv[i], sizeof(input_file_base_name) - 1);
        input_file_base_name[sizeof(input_file_base_name) - 1] = '\0';
        
        sprintf(full_input_file_name, "%s.as", input_file_base_name);

        g_has_error = 0; /* Reset for each new file*/

        printf("\n--- Processing file: %s ---\n", full_input_file_name);

        input_file_stream = fopen(full_input_file_name, "r");
        if (!input_file_stream) {
            fprintf(stderr, "Error: Cannot open input file: %s. Skipping.\n", full_input_file_name);
            continue; /* Skip to the next file*/
        }

        /* --- 1. Macro Processing ---*/
        macro_list = processMacroDefinitions(input_file_stream);
        if (g_has_error) {
            fprintf(stderr, "Errors found during macro definition processing for %s. Halting assembly for this file.\n", full_input_file_name);
            fclose(input_file_stream);
            freeMacroList(macro_list);
            continue; /* Skip to the next file */
        }

        sprintf(am_file_name, "%s.am", input_file_base_name);
        am_file_stream = fopen(am_file_name, "w+");
        if (!am_file_stream) {
            fprintf(stderr, "Error: Cannot create .am file: %s. Halting assembly for this file.\n", am_file_name);
            fclose(input_file_stream);
            freeMacroList(macro_list);
            continue; /* Skip to the next file */
        }
        
        rewind(input_file_stream);
        
        while (fgets(line, sizeof(line), input_file_stream)) {
            const char *trimmed = skip_whitespace_macro(line);
            
            /* Extract the first word from the line to check for macro keywords */
            char first_word[MAX_SYMBOL_LENGTH];
            if (sscanf(trimmed, "%s", first_word) == 1) {
                /* Check if this is the start of a macro definition */
                if (strcmp(first_word, "mcro") == 0) {
                    inside_macro_def = 1; /* Set flag - we're now inside a macro */
                    continue; /* Skip the "mcro" line itself */
                }
                /* Check if this is the end of a macro definition */
                else if (strcmp(first_word, "mcroend") == 0) {
                    inside_macro_def = 0; /* Clear flag - macro definition ended */
                    continue; /* Skip the "mcroend" line itself */
                }
            }
            
            /* If we're inside a macro definition, skip this line entirely */
            /* This ensures macro body lines don't appear in the .am file */
            if (inside_macro_def) {
                continue;
            }
            
            /* For all other lines (outside macro definitions), expand any macro calls */
            expanded_line_content = expandMacroInLine(line, macro_list);

            /* First, write the content to the .am file */
            if (strlen(expanded_line_content) > 0 && 
                expanded_line_content[strlen(expanded_line_content) - 1] != '\n') {
                fprintf(am_file_stream, "%s\n", expanded_line_content);
            } else {
                fprintf(am_file_stream, "%s", expanded_line_content);
            }

            /*  free the memory ONLY if a new string was allocated */
            if (expanded_line_content != line) {
                free(expanded_line_content);
            }
        }
        freeMacroList(macro_list);
        fclose(input_file_stream);
        rewind(am_file_stream);

        /* --- 2. First Pass ---*/
        if (!firstPass(am_file_stream, &symbol_table, &instruction_list, &data_list, &final_ic, &final_dc)) {
            g_has_error = 1; /* Ensure error is flagged*/
        }
        fclose(am_file_stream);

        /* --- 3. Second Pass ---*/
        if (!g_has_error) {
            if (!secondPass(instruction_list, symbol_table)) {
                g_has_error = 1; /* Ensure error is flagged*/
            }
        }
        
        /* --- 4. Write Output Files  --- */
        if (g_has_error) {
            fprintf(stderr, "Errors detected during assembly. No output files generated for %s.\n", input_file_base_name);
        } else {
            printf("Generating output files for %s...\n", input_file_base_name);
            
            /* Pass just the base name to the output functions - they will add extensions */
            /* The .ob file header requires the LENGTH of the instruction code, not the final address. */
            writeObjectFile(input_file_base_name, final_ic - MEMORY_START, final_dc, data_list, instruction_list);
            writeEntriesFile(input_file_base_name, symbol_table);
            writeExternalsFile(input_file_base_name, symbol_table);
        }

        /* --- 5. Memory Cleanup for this file's data --- */
        freeSymbolTable(symbol_table);
        while (instruction_list) {
            tempInst = instruction_list;
            instruction_list = instruction_list->next;
            free(tempInst);
        }
        while (data_list) {
            tempData = data_list;
            data_list = data_list->next;
            free(tempData);
        }
        printf("--- Finished processing %s ---\n", full_input_file_name);
    } /* End of loop for processing files */

    return 0; /* Main returns 0, success/failure is per-file. */
}