/* macro_processor.c */
/**
 * @file macro_processor.c
 * @brief Implements the assembler's macro processing module.
 *
 * This module handles the pre-processing stage of assembly:
 * 1. Reads and stores macro definitions from the source file
 * 2. Expands macro calls into their defined content
 * 3. Outputs an expanded file (.am) for the main assembler to process
 * 
 * Macros allow code reuse by defining named blocks of assembly code
 * that can be inserted wherever the macro name appears.
 */

#define INITIAL_MACRO_LINES_CAPACITY 10  /* Starting size for macro line storage */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "macro_processor.h"
#include "assembler.h"

/* --- Internal Helper Functions Prototypes --- */
Macro* processMacroDefinitions(FILE* input);
char* expandMacroInLine(const char* line, Macro* macroList);
void freeMacroList(Macro* head);
static char* skip_whitespace_macro(char* s);
static int is_reserved_macro_name(const char* name);

/**
 * Main entry point for macro processing stage
 * This is called before the two-pass assembly begins
 * 
 * Process:
 * 1. First pass through file: collect all macro definitions
 * 2. Second pass through file: expand macro calls and write output
 * 
 * @param input  Input file stream (.as file with possible macros)
 * @param output Output file stream (.am file with macros expanded)
 * @return 1 on success, 0 on failure
 */
int create_expanded_file(FILE* input, FILE* output) {
    char line[MAX_LINE_LENGTH + 2];  /* Buffer for reading lines */
    int in_macro_def_for_skipping = 0;  /* Flag: currently inside macro definition */
    Macro* macroList;  /* Linked list of all macro definitions */
    char* trimmed_line;
    char first_word[MAX_SYMBOL_LENGTH] = {0};
    char *expanded;

    /* Step 1: First pass - collect all macro definitions */
    macroList = processMacroDefinitions(input);
    if (g_has_error) {
        /* Error occurred during macro processing */
        freeMacroList(macroList);
        return 0;
    }

    /* Step 2: Reset file to beginning for second pass */
    rewind(input);

    /* Step 3: Second pass - expand macros and write output */
    while (fgets(line, sizeof(line), input)) {
        trimmed_line = skip_whitespace_macro(line);

        /* Check if we're entering or leaving a macro definition */
        /* We skip these in the output since macros are being expanded */
        if (sscanf(trimmed_line, "%s", first_word) == 1) {
            if (strcmp(first_word, "mcro") == 0) {
                /* Start of macro definition - skip it */
                in_macro_def_for_skipping = 1;
                continue;
            }
            if (strcmp(first_word, "mcroend") == 0) {
                /* End of macro definition - stop skipping */
                in_macro_def_for_skipping = 0;
                continue;
            }
        }
        
        /* Skip lines inside macro definitions */
        if (in_macro_def_for_skipping) continue;

        /* Try to expand any macro calls in this line */
        expanded = expandMacroInLine(line, macroList);

        /* Write the result (either expanded or original) to output */
        fprintf(output, "%s", expanded);
        
        /* Ensure line ends with newline */
        if (expanded[strlen(expanded)-1] != '\n') {
            fprintf(output, "\n");
        }

        /* Free memory only if expansion created a new string */
        if (expanded != line) {
            free(expanded);
        }
    }

    /* Step 4: Clean up all macro definitions */
    freeMacroList(macroList);
    return 1;
}

/**
 * Utility function to skip whitespace at beginning of string
 * Used throughout macro processing for parsing
 * 
 * @param s Input string pointer
 * @return Pointer to first non-whitespace character
 */
char* skip_whitespace_macro(char* s) {
    while (s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/**
 * Checks if a name is reserved and cannot be used as a macro name
 * Prevents conflicts with assembly instructions and directives
 * 
 * @param name The proposed macro name
 * @return 1 if reserved (cannot use), 0 if available
 */
int is_reserved_macro_name(const char* name) {
    /* Check if it's an opcode (mov, add, etc.) */
    if (is_opcode(name) || is_register(name)) return 1;
    
    /* Check if it's a directive or macro keyword */
    if (strcmp(name, ".data") == 0 || strcmp(name, ".string") == 0 ||
        strcmp(name, ".mat") == 0 || strcmp(name, ".extern") == 0 ||
        strcmp(name, ".entry") == 0 || strcmp(name, "mcro") == 0 ||
        strcmp(name, "mcroend") == 0) {
        return 1;
    }
    return 0;
}

/**
 * First pass: Reads entire file and collects all macro definitions
 * Builds a linked list of Macro structures containing macro names and content
 * 
 * Macro syntax:
 *   mcro MACRO_NAME
 *   ... macro content ...
 *   mcroend
 * 
 * @param input The input file to scan for macros
 * @return Head of linked list containing all macro definitions
 */
Macro* processMacroDefinitions(FILE* input) {
    Macro* macroList = NULL;  /* Head of macro list */
    char line[MAX_LINE_LENGTH + 2];
    int inMacro = 0;  /* Flag: currently inside a macro definition */
    Macro *currentMacro = NULL;  /* Macro being built */
    int lineNumber = 0;
    int c;
    char *macro_name_pos;
    char macro_name[MAX_SYMBOL_LENGTH];
    char *remaining;
    Macro *temp_iter;
    char **new_lines;  /* For reallocation when macro grows */
    size_t line_len;
    char *trimmed_line;
    char command_token[MAX_SYMBOL_LENGTH];
    int read_count;
    int name_read_count;

    /* Process file line by line */
    while (fgets(line, sizeof(line), input)) {
        lineNumber++;
        
        /* Check for line length overflow */
        line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] != '\n' && !feof(input)) {
            fprintf(stderr, "Error at line %d: Line exceeds maximum length.\n", lineNumber);
            g_has_error = 1;
            /* Skip rest of oversized line */
            while ((c = fgetc(input)) != '\n' && c != EOF);
            continue;
        }

        /* Remove newline characters */
        line[strcspn(line, "\r\n")] = '\0';
        trimmed_line = skip_whitespace_macro(line);

        /* Skip empty lines and comments */
        if (*trimmed_line == '\0' || *trimmed_line == ';') continue;

        /* Parse first word of line */
        if (sscanf(trimmed_line, "%s%n", command_token, &read_count) != 1) continue;

        /* Check for macro definition start */
        if (strcmp(command_token, "mcro") == 0) {
            /* Validate we're not already in a macro (no nesting allowed) */
            if (inMacro) {
                fprintf(stderr, "Error at line %d: Nested macro definitions are not allowed.\n", lineNumber);
                g_has_error = 1;
                continue;
            }
            inMacro = 1;
            
            /* Extract macro name */
            macro_name_pos = skip_whitespace_macro(trimmed_line + read_count);

            if (sscanf(macro_name_pos, "%s%n", macro_name, &name_read_count) != 1) {
                fprintf(stderr, "Error at line %d: Macro definition missing name.\n", lineNumber);
                g_has_error = 1;
                inMacro = 0; 
                continue;
            }

            /* Check for extra text after macro name */
            remaining = skip_whitespace_macro(macro_name_pos + name_read_count);
            if (*remaining != '\0') {
                fprintf(stderr, "Error at line %d: Extraneous text after macro name.\n", lineNumber);
                g_has_error = 1;
                inMacro = 0; 
                continue;
            }

            /* Validate macro name */
            if (!is_valid_label(macro_name) || is_reserved_macro_name(macro_name)) {
                fprintf(stderr, "Error at line %d: Invalid or reserved macro name '%s'.\n", lineNumber, macro_name);
                g_has_error = 1;
                inMacro = 0; 
                continue;
            }

            /* Check for duplicate macro names */
            temp_iter = macroList;
            while(temp_iter){
                if(strcmp(temp_iter->name, macro_name) == 0){
                    fprintf(stderr, "Error at line %d: Macro '%s' already defined.\n", lineNumber, macro_name);
                    g_has_error = 1; 
                    break;
                }
                temp_iter = temp_iter->next;
            }
            if(g_has_error) { 
                inMacro = 0; 
                continue; 
            }

            /* Create new macro structure */
            currentMacro = (Macro*)malloc(sizeof(Macro));
            if (!currentMacro) exit(1);

            /* Initialize macro */
            strncpy(currentMacro->name, macro_name, MAX_SYMBOL_LENGTH -1);
            currentMacro->name[MAX_SYMBOL_LENGTH - 1] = '\0';
            currentMacro->lineCount = 0;
            currentMacro->capacity = INITIAL_MACRO_LINES_CAPACITY;
            
            /* Allocate initial line storage */
            currentMacro->lines = malloc(currentMacro->capacity * sizeof(char*));
            if (!currentMacro->lines) { 
                free(currentMacro); 
                exit(1); 
            }

            /* Add to macro list */
            currentMacro->next = macroList;
            macroList = currentMacro;

        } else if (strcmp(command_token, "mcroend") == 0) {
            /* End of macro definition */
            if (!inMacro) {
                fprintf(stderr, "Error at line %d: 'mcroend' without 'mcro'.\n", lineNumber);
                g_has_error = 1; 
                continue;
            }
            
            /* Check for extra text after mcroend */
            if (skip_whitespace_macro(trimmed_line + read_count)[0] != '\0') {
                fprintf(stderr, "Error at line %d: Extraneous text after 'mcroend'.\n", lineNumber);
                g_has_error = 1;
            }
            
            /* Close current macro definition */
            inMacro = 0;
            currentMacro = NULL;

        } else if (inMacro) {
            /* Inside macro definition - store this line */
            if (!currentMacro) { 
                g_has_error = 1; 
                inMacro = 0; 
                continue; 
            }
            
            /* Expand storage if needed (dynamic array growth) */
            if (currentMacro->lineCount >= currentMacro->capacity) {
                currentMacro->capacity *= 2;  /* Double capacity */
                new_lines = realloc(currentMacro->lines, currentMacro->capacity * sizeof(char*));
                if (!new_lines) exit(1);
                currentMacro->lines = new_lines;
            }
            
            /* Store complete line including original indentation */
            /* Using malloc+strcpy instead of strdup for ANSI C compliance */
            currentMacro->lines[currentMacro->lineCount] = malloc(strlen(line) + 1);
            if (!currentMacro->lines[currentMacro->lineCount]) exit(1);
            strcpy(currentMacro->lines[currentMacro->lineCount], line);
            currentMacro->lineCount++;
        }
    }

    /* Check for unclosed macro definition */
    if (inMacro) {
        fprintf(stderr, "Error: Macro definition started but never ended with 'mcroend'.\n");
        g_has_error = 1;
    }

    return macroList;
}

/**
 * Expands macro calls in a single line
 * Handles labels that may appear before macro calls (e.g., "LABEL: macro_name")
 * 
 * @param line The line to check for macro expansion
 * @param macroList List of all defined macros
 * @return Pointer to expanded string (newly allocated) or original line if no macro
 */
char* expandMacroInLine(const char* line, Macro* macroList) {
    char line_copy[MAX_LINE_LENGTH + 2];
    char *trimmed;
    char *colon;
    char macro_name[MAX_SYMBOL_LENGTH];
    char *macro_start;
    Macro *iter;
    int leading_spaces;
    int i, j;
    char *result;
    size_t total_size;
    size_t prefix_len;
    
    if (!line) return (char*)line;
    
    /* Work with a copy to avoid modifying original */
    strcpy(line_copy, line);
    
    /* Remove newline for processing */
    line_copy[strcspn(line_copy, "\r\n")] = '\0';
    
    trimmed = skip_whitespace_macro(line_copy);
    
    /* Skip empty lines and comments */
    if (*trimmed == '\0' || *trimmed == ';') return (char*)line;
    
    /* Check for label (indicated by colon) */
    colon = strchr(trimmed, ':');
    macro_start = trimmed;
    
    if (colon) {
        /* Line has a label - macro name would be after the colon */
        macro_start = skip_whitespace_macro(colon + 1);
        if (*macro_start == '\0') return (char*)line;
    }
    
    /* Extract the potential macro name (first word after label if present) */
    i = 0;
    while (macro_start[i] && !isspace(macro_start[i]) && i < MAX_SYMBOL_LENGTH - 1) {
        macro_name[i] = macro_start[i];
        i++;
    }
    macro_name[i] = '\0';
    
    if (macro_name[0] == '\0') return (char*)line;
    
    /* Search for matching macro */
    for (iter = macroList; iter != NULL; iter = iter->next) {
        if (strcmp(iter->name, macro_name) == 0) {
            /* Found a macro to expand */
            
            /* Calculate total size needed for expansion */
            total_size = 1;  /* For null terminator */
            prefix_len = 0;
            
            /* If there's a label, we need to preserve it */
            if (colon) {
                /* Calculate length from start to where macro name starts */
                prefix_len = macro_start - trimmed + (trimmed - line_copy);
                /* Include original leading whitespace */
                leading_spaces = trimmed - line_copy;
                prefix_len += leading_spaces;
                total_size += prefix_len;
            }
            
            /* Add size for all macro lines */
            for (j = 0; j < iter->lineCount; j++) {
                total_size += strlen(iter->lines[j]) + 1;  /* +1 for newline */
            }
            
            /* Allocate result buffer */
            result = (char*)malloc(total_size);
            if (!result) return (char*)line;
            
            result[0] = '\0';
            
            /* Build the expanded result */
            if (colon && iter->lineCount > 0) {
                /* Preserve label, replace macro name with first macro line */
                strncat(result, line, macro_start - line_copy);
                
                /* Add first macro line after label */
                strcat(result, iter->lines[0]);
                
                /* Add remaining macro lines (without label) */
                for (j = 1; j < iter->lineCount; j++) {
                    strcat(result, "\n");
                    strcat(result, iter->lines[j]);
                }
            } else {
                /* No label - just expand the macro lines */
                for (j = 0; j < iter->lineCount; j++) {
                    if (j > 0) strcat(result, "\n");
                    strcat(result, iter->lines[j]);
                }
            }
            
            return result;  /* Return newly allocated expanded string */
        }
    }
    
    /* No macro found - return original line */
    return (char*)line;
}

/**
 * Frees all memory used by the macro list
 * Cleans up the linked list and all stored macro lines
 * 
 * @param head The head of the macro list to free
 */
void freeMacroList(Macro* head) {
    Macro *temp;
    int i;
    
    /* Walk through linked list */
    while (head) {
        /* Free all stored lines in this macro */
        for (i = 0; i < head->lineCount; i++) {
            free(head->lines[i]);
        }
        /* Free the lines array itself */
        free(head->lines);
        
        /* Move to next and free current node */
        temp = head;
        head = head->next;
        free(temp);
    }
}
