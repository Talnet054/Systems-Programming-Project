/* first_pass.c */
/**
 * @file first_pass.c
 * @brief Implements the assembler's first pass logic.
 * 
 * The first pass of the assembler:
 * 1. Builds the symbol table with labels and their addresses
 * 2. Calculates instruction lengths and addresses
 * 3. Validates syntax and operands
 * 4. Prepares data structures for the second pass
 */

#define _POSIX_C_SOURCE 200809L
#include "first_pass.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --- External Dependencies --- */
/* Global error flag - set when any error occurs during assembly */
extern int g_has_error;

/* Symbol table management functions from symbol_table.c */
extern void addSymbol(Symbol **head, const char *name, int address, SymbolType type, int line_num);
extern Symbol* findSymbol(Symbol *head, const char *name);
extern void updateDataSymbolsAddresses(Symbol *head, int icf);

/* Base-4 conversion function from second_pass.c */
extern char* get_addressing_mode_base4(const char* operand);

/* --- Local Helper Function Prototypes --- */
char* skip_whitespace(char* s);
int is_valid_number(const char *s);
int is_opcode(const char* s);
int is_register(const char* s);
int is_valid_label(const char* s);
static int validate_data_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr);
static int validate_string_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr);
static int validate_mat_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr);
int validate_instruction_operands(const char* opcode, const char* operand1_str, const char* operand2_str, int num_operands_found, int line_num);

/* --- Helper Functions Implementations --- */

/**
 * Skips whitespace characters from the beginning of a string
 * @param s Input string pointer
 * @return Pointer to first non-whitespace character
 */
char* skip_whitespace(char* s) {
    while (s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/**
 * Validates if a string represents a valid integer number
 * @param s String to validate
 * @return 1 if valid number, 0 otherwise
 */
int is_valid_number(const char *s) {
    char *endptr;
    
    /* Empty string is not a valid number */
    if (!s || *s == '\0') {
        return 0;
    }
    
    /* Try to parse the number, endptr will point to first non-digit character */
    strtol(s, &endptr, 10);
    
    /* Valid only if entire string was consumed */
    return (*endptr == '\0');
}

/**
 * Checks if a string is a valid assembly opcode
 * @param s String to check
 * @return 1 if valid opcode, 0 otherwise
 */
int is_opcode(const char* s) {
    /* Array of all valid opcodes in our assembly language */
    const char *opcodes[] = {
        "mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", "dec", "jmp", "bne",
        "red", "prn", "jsr", "rts", "stop", NULL
    };
    int i;
    
    /* Compare with each valid opcode */
    for (i = 0; opcodes[i]; i++) {
        if (strcmp(s, opcodes[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Checks if a string represents a valid register (r0-r7)
 * @param s String to check
 * @return 1 if valid register, 0 otherwise
 */
int is_register(const char* s) {
    int reg_num;
    
    /* Register must be exactly 2 chars: 'r' followed by digit */
    if (!s || strlen(s) != 2 || s[0] != 'r' || !isdigit((unsigned char)s[1])) 
        return 0;
    
    /* Extract register number and validate range 0-7 */
    reg_num = atoi(s + 1);
    return (reg_num >= 0 && reg_num <= 7);
}

/**
 * Validates if a string can be used as a label
 * Rules: Must start with letter, contain only alphanumeric chars, max length 30
 * @param s String to validate
 * @return 1 if valid label, 0 otherwise
 */
int is_valid_label(const char* s) {
    int i;
    
    /* Check for null, empty, or too long */
    if (!s || *s == '\0' || strlen(s) >= MAX_SYMBOL_LENGTH) 
        return 0;
    
    /* First character must be alphabetic */
    if (!isalpha((unsigned char)s[0])) 
        return 0;
    
    /* Rest must be alphanumeric */
    for (i = 1; s[i] != '\0'; i++) {
        if (!isalnum((unsigned char)s[i])) 
            return 0;
    }
    return 1;
}

/**
 * Calculates the number of memory words an instruction will occupy
 * @param opcode The instruction opcode
 * @param op1 First operand (may be NULL)
 * @param op2 Second operand (may be NULL)
 * @return Number of words needed, or -1 on error
 */
int calculate_instruction_length(const char* opcode, const char* op1, const char* op2) {
    int expected_operands = -1;
    int m_dummy, n_dummy;  /* For matrix parsing */
    int length = 1;  /* Base instruction always takes 1 word */
    int num_operands_parsed = 0;
    int op1_is_reg = 0, op2_is_reg = 0;
    int op1_extra_words = 0, op2_extra_words = 0;

    /* Count how many operands were provided */
    if (op1 && op1[0] != '\0') num_operands_parsed++;
    if (op2 && op2[0] != '\0') num_operands_parsed++;

    /* Determine expected operand count based on opcode */
    /* No operand instructions */
    if (strcmp(opcode, "rts") == 0 || strcmp(opcode, "stop") == 0) 
        expected_operands = 0;
    /* Single operand instructions */
    else if (strcmp(opcode, "not") == 0 || strcmp(opcode, "clr") == 0 ||
             strcmp(opcode, "inc") == 0 || strcmp(opcode, "dec") == 0 ||
             strcmp(opcode, "jmp") == 0 || strcmp(opcode, "bne") == 0 ||
             strcmp(opcode, "jsr") == 0 || strcmp(opcode, "red") == 0 ||
             strcmp(opcode, "prn") == 0)
        expected_operands = 1;
    /* Two operand instructions */
    else if (strcmp(opcode, "mov") == 0 || strcmp(opcode, "cmp") == 0 ||
             strcmp(opcode, "add") == 0 || strcmp(opcode, "sub") == 0 ||
             strcmp(opcode, "lea") == 0)
        expected_operands = 2;

    /* Validate operand count */
    if (expected_operands == -1 || expected_operands != num_operands_parsed)
        return -1;

    /* Calculate extra words needed for first operand */
    if (num_operands_parsed >= 1) {
        if (op1[0] == '#') {
            /* Immediate addressing - needs 1 extra word */
            op1_extra_words = 1;
        } else if (strchr(op1, '[')) {
            /* Matrix addressing - needs 2 extra words for indices */
            if (sscanf(op1, "%*[^[][r%d][r%d]", &m_dummy, &n_dummy) == 2) {
                op1_extra_words = 2;
            } else {
                return -1;  /* Invalid matrix format */
            }
        } else if (is_register(op1)) {
            /* Register addressing */
            op1_is_reg = 1;
            op1_extra_words = 1;
        } else {
            /* Direct addressing (label) - needs 1 extra word */
            op1_extra_words = 1;
        }
    }

    /* Calculate extra words needed for second operand */
    if (num_operands_parsed == 2) {
        if (op2[0] == '#') {
            /* Immediate addressing */
            op2_extra_words = 1;
        } else if (strchr(op2, '[')) {
            /* Matrix addressing */
            if (sscanf(op2, "%*[^[][r%d][r%d]", &m_dummy, &n_dummy) == 2) {
                op2_extra_words = 2;
            } else {
                return -1;
            }
        } else if (is_register(op2)) {
            /* Register addressing */
            op2_is_reg = 1;
            op2_extra_words = 1;
        } else {
            /* Direct addressing (label) */
            op2_extra_words = 1;
        }
    }

    /* Special case: two registers can share one word */
    if (op1_is_reg && op2_is_reg) {
        return length + 1;  /* Base + 1 word for both registers */
    }

    /* Calculate total length */
    length += op1_extra_words + op2_extra_words;

    /* Sanity check - instruction shouldn't exceed 5 words */
    if (length > 5) {
        return -1;
    }
    return length;
}

/**
 * Validates and processes .data directive parameters
 * Parses comma-separated integers and adds them to data list
 * @param params_str Parameter string after .data
 * @param line_num Line number for error reporting
 * @param temp_data_head Head of temporary data list
 * @param DC_ptr Pointer to data counter
 * @return 1 on success, 0 on error
 */
static int validate_data_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr) {
    char *token;
    char *rest = params_str;
    int success = 1;
    char *trimmed;
    int value;
    DataItem *newData;

    /* Check for empty parameters */
    if (params_str == NULL || *skip_whitespace(params_str) == '\0') {
        fprintf(stderr, "Error at line %d: Missing parameters for .data directive.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Check for leading comma */
    rest = skip_whitespace(rest);
    if (*rest == ',') {
        fprintf(stderr, "Error at line %d: Leading comma in .data directive parameters.\n", line_num);
        success = 0;
        rest++;
    }

    /* Parse each comma-separated value */
    while ((token = strtok_r(rest, ",", &rest))) {
        /* Trim whitespace from token */
        trimmed = skip_whitespace(token);
        trimmed[strcspn(trimmed, " \t")] = '\0';

        /* Check for empty token (consecutive commas) */
        if (strlen(trimmed) == 0) {
            fprintf(stderr, "Error at line %d: Invalid empty parameter or multiple consecutive commas in .data.\n", line_num);
            success = 0;
            continue;
        }

        /* Validate number format */
        if (!is_valid_number(trimmed)) {
            fprintf(stderr, "Error at line %d: Invalid number '%s' in .data directive.\n", line_num, trimmed);
            success = 0;
            continue;
        }
        
        /* Parse the integer value */
        value = atoi(trimmed);

        /* Check value range (-512 to 511 for 10-bit numbers) */
        if (value < -512 || value > 511) {
            fprintf(stderr, "Error at line %d: Data value %d out of range [-512, 511] in .data directive.\n", line_num, value);
            success = 0;
            continue;
        }

        /* Create new data item and add to list */
        newData = (DataItem *)malloc(sizeof(DataItem));
        if (!newData) { 
            exit(1); 
        }
        newData->address = (*DC_ptr)++;  /* Assign address and increment counter */
        newData->value = value;
        newData->next = *temp_data_head;  /* Add to front of list */
        *temp_data_head = newData;
    }

    if (!success) 
        g_has_error = 1;
    return success;
}

/**
 * Validates and processes .string directive parameters
 * Converts string to individual character values in data list
 * @param params_str Parameter string after .string
 * @param line_num Line number for error reporting
 * @param temp_data_head Head of temporary data list
 * @param DC_ptr Pointer to data counter
 * @return 1 on success, 0 on error
 */
static int validate_string_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr) {
    char *start = skip_whitespace(params_str);
    char *end;
    char *str_content;
    DataItem *newData;
    DataItem *nullTerm;

    /* String must start with quote */
    if (*start != '"') {
        fprintf(stderr, "Error at line %d: String must begin with a quote.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Find closing quote */
    end = strrchr(start + 1, '"');
    if (!end) {
        fprintf(stderr, "Error at line %d: String must end with a quote.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Check for text after closing quote */
    if (*skip_whitespace(end + 1) != '\0') {
        fprintf(stderr, "Error at line %d: Extraneous text after string.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Extract string content (between quotes) */
    *end = '\0';
    str_content = start + 1;

    /* Add each character as a data item */
    while (*str_content) {
        newData = (DataItem *)malloc(sizeof(DataItem));
        if (!newData) { 
            exit(1); 
        }
        newData->address = (*DC_ptr)++;
        newData->value = (int)*str_content;  /* ASCII value of character */
        newData->next = *temp_data_head;
        *temp_data_head = newData;
        str_content++;
    }

    /* Add null terminator */
    nullTerm = (DataItem *)malloc(sizeof(DataItem));
    if (!nullTerm) { 
        exit(1); 
    }
    nullTerm->address = (*DC_ptr)++;
    nullTerm->value = 0;  /* Null terminator */
    nullTerm->next = *temp_data_head;
    *temp_data_head = nullTerm;

    return 1;
}

/**
 * Validates and processes .mat (matrix) directive parameters
 * Parses matrix dimensions and initial values
 * @param params_str Parameter string after .mat
 * @param line_num Line number for error reporting
 * @param temp_data_head Head of temporary data list
 * @param DC_ptr Pointer to data counter
 * @return 1 on success, 0 on error
 */
static int validate_mat_parameters(char *params_str, int line_num, DataItem **temp_data_head, int *DC_ptr) {
    int success = 1;
    int rows, cols;
    int read_count_dims;
    char *current_val_pos = skip_whitespace(params_str);
    int numCells;
    int count_initialized_values = 0;
    char *num_start_ptr;
    char *num_end_ptr;
    char num_str_val[MAX_LINE_LENGTH];
    size_t num_len;
    int value;
    DataItem *newData;

    /* Parse matrix dimensions [rows][cols] */
    if (sscanf(current_val_pos, " [ %d ] [ %d ]%n", &rows, &cols, &read_count_dims) != 2) {
        fprintf(stderr, "Error at line %d: Invalid or missing matrix dimensions. Expected '[rows][cols]'.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Move past the dimensions */
    current_val_pos += read_count_dims;
    current_val_pos = skip_whitespace(current_val_pos);

    /* Validate dimensions are positive */
    if (rows <= 0 || cols <= 0) {
        fprintf(stderr, "Error at line %d: Matrix dimensions must be positive integers.\n", line_num);
        g_has_error = 1; 
        return 0;
    }

    /* Calculate total cells in matrix */
    numCells = rows * cols;
    num_start_ptr = current_val_pos;

    /* Check for leading comma */
    if (*num_start_ptr == ',') {
        fprintf(stderr, "Error at line %d: Leading comma in .mat initialization parameters.\n", line_num);
        success = 0;
        num_start_ptr++;
    }
    num_start_ptr = skip_whitespace(num_start_ptr);

    /* Parse initialization values */
    while (*num_start_ptr != '\0' && count_initialized_values < numCells) {
        num_end_ptr = num_start_ptr;
        
        /* Handle sign */
        if (*num_end_ptr == '+' || *num_end_ptr == '-') 
            num_end_ptr++;
        
        /* Check for valid digit */
        if (!isdigit((unsigned char)*num_end_ptr)) {
            fprintf(stderr, "Error at line %d: Invalid character in .mat initialization. Expected a number.\n", line_num);
            success = 0; 
            break;
        }
        
        /* Find end of number */
        while (isdigit((unsigned char)*num_end_ptr)) 
            num_end_ptr++;

        /* Extract number string */
        num_len = num_end_ptr - num_start_ptr;
        if (num_len == 0 || num_len >= MAX_LINE_LENGTH) {
            fprintf(stderr, "Error at line %d: Invalid number format or length in .mat.\n", line_num);
            success = 0; 
            break;
        }
        strncpy(num_str_val, num_start_ptr, num_len);
        num_str_val[num_len] = '\0';

        /* Validate and parse number */
        if (!is_valid_number(num_str_val)) {
            fprintf(stderr, "Error at line %d: Invalid number format in .mat initialization: '%s'.\n", line_num, num_str_val);
            success = 0; 
            break;
        }
        value = atoi(num_str_val);

        /* Check value range */
        if (value < -512 || value > 511) {
            fprintf(stderr, "Error at line %d: Data value %d out of range [-512, 511] in .mat directive.\n", line_num, value);
            success = 0; 
            break;
        }

        /* Add value to data list */
        newData = (DataItem *)malloc(sizeof(DataItem));
        if (!newData) { 
            exit(1); 
        }
        newData->address = (*DC_ptr)++;
        newData->value = value;
        newData->next = *temp_data_head;
        *temp_data_head = newData;
        count_initialized_values++;

        /* Move to next value */
        num_start_ptr = num_end_ptr;
        num_start_ptr = skip_whitespace(num_start_ptr);

        /* Handle comma separator */
        if (*num_start_ptr == ',') {
            num_start_ptr++;
            num_start_ptr = skip_whitespace(num_start_ptr);
            
            /* Check for trailing comma */
            if (*num_start_ptr == '\0') {
                fprintf(stderr, "Error at line %d: Trailing comma in .mat initialization parameters.\n", line_num);
                success = 0; 
                break;
            }
            
            /* Check for consecutive commas */
            if (*num_start_ptr == ',') {
                fprintf(stderr, "Error at line %d: Multiple consecutive commas in .mat initialization parameters.\n", line_num);
                success = 0; 
                break;
            }
        } else if (*num_start_ptr != '\0') {
            /* Missing comma between values */
            fprintf(stderr, "Error at line %d: Expected comma or end of line after number in .mat initialization.\n", line_num);
            success = 0; 
            break;
        }
    }

    /* Fill remaining cells with zeros */
    while (count_initialized_values < numCells) {
        newData = (DataItem *)malloc(sizeof(DataItem));
        if (!newData) { 
            exit(1); 
        }
        newData->address = (*DC_ptr)++;
        newData->value = 0;  /* Default value */
        newData->next = *temp_data_head;
        *temp_data_head = newData;
        count_initialized_values++;
    }

    /* Warn about extra values */
    if (success && *num_start_ptr != '\0') {
        fprintf(stderr, "Warning at line %d: Extraneous text or too many initialization values for .mat directive. Excess values ignored.\n", line_num);
    }

    if (!success) 
        g_has_error = 1;
    return success;
}

/**
 * Validates instruction operands against allowed addressing modes
 * Each instruction has specific allowed addressing modes for its operands
 * @param opcode The instruction opcode
 * @param op1 First operand string
 * @param op2 Second operand string  
 * @param num_ops Number of operands found
 * @param line Line number for error reporting
 * @return 1 if valid, 0 if invalid
 */
int validate_instruction_operands(const char* opcode, const char* op1, const char* op2, int num_ops, int line) {
    int opcode_num = -1;
    int expected_operands = 0;
    char actual_src_mode_char;
    char actual_dest_mode_char;
    int success = 1;
    
    /* Table of legal addressing modes for each instruction */
    /* 'a'=immediate, 'b'=direct, 'c'=matrix, 'd'=register, 'X'=no operand */
    const char* legal_modes[16][2]={
        {"abcd", "bcd"}, /* mov - src: all modes, dest: no immediate */
        {"abcd", "abcd"}, /* cmp - src: all modes, dest: all modes */
        {"abcd", "bcd"}, /* add */
        {"abcd", "bcd"}, /* sub */
        {"X", "bcd"}, /* not - single operand */
        {"X", "bcd"}, /* clr */
        {"bc", "bcd"}, /* lea - src: no immediate/register */
        {"X", "bcd"}, /* inc */
        {"X", "bcd"}, /* dec */
        {"X", "bc"}, /* jmp - no register */
        {"X", "bc"}, /* bne */
        {"X", "bcd"}, /* red */
        {"X", "abcd"}, /* prn - can print anything */
        {"X", "bc"}, /* jsr */
        {"X", "X"}, /* rts - no operands */
        {"X", "X"} /* stop - no operands */
    };

    /* Get addressing mode characters for operands */
    actual_src_mode_char = get_addressing_mode_base4(op1)[0];
    actual_dest_mode_char = get_addressing_mode_base4(op2)[0];

    /* Map opcode string to number */
    if (strcmp(opcode, "mov") == 0) opcode_num = 0;
    else if (strcmp(opcode, "cmp") == 0) opcode_num = 1;
    else if (strcmp(opcode, "add") == 0) opcode_num = 2;
    else if (strcmp(opcode, "sub") == 0) opcode_num = 3;
    else if (strcmp(opcode, "not") == 0) opcode_num = 4;
    else if (strcmp(opcode, "clr") == 0) opcode_num = 5;
    else if (strcmp(opcode, "lea") == 0) opcode_num = 6;
    else if (strcmp(opcode, "inc") == 0) opcode_num = 7;
    else if (strcmp(opcode, "dec") == 0) opcode_num = 8;
    else if (strcmp(opcode, "jmp") == 0) opcode_num = 9;
    else if (strcmp(opcode, "bne") == 0) opcode_num = 10;
    else if (strcmp(opcode, "red") == 0) opcode_num = 11;
    else if (strcmp(opcode, "prn") == 0) opcode_num = 12;
    else if (strcmp(opcode, "jsr") == 0) opcode_num = 13;
    else if (strcmp(opcode, "rts") == 0) opcode_num = 14;
    else if (strcmp(opcode, "stop") == 0) opcode_num = 15;

    /* Validate opcode was found */
    if (opcode_num == -1) {
        fprintf(stderr, "Internal Error: Unknown opcode '%s' in operand validation.\n", opcode);
        g_has_error = 1; 
        return 0;
    }

    /* Bounds check */
    if (opcode_num >= (int)(sizeof(legal_modes) / sizeof(legal_modes[0]))) {
        fprintf(stderr, "Internal Error: Opcode number %d out of bounds for legal_modes array.\n", opcode_num);
        g_has_error = 1; 
        return 0;
    }
    
    /* Count expected operands based on legal modes table */
    if (strcmp(legal_modes[opcode_num][0], "X") != 0) 
        expected_operands++;
    if (strcmp(legal_modes[opcode_num][1], "X") != 0) 
        expected_operands++;

    /* Check operand count matches expectation */
    if (expected_operands != num_ops) {
        fprintf(stderr, "Error at line %d: Instruction '%s' expects %d operands, but %d were found.\n", 
                line, opcode, expected_operands, num_ops);
        g_has_error = 1; 
        return 0;
    }

    /* Validate source operand addressing mode (for 2-operand instructions) */
    if (num_ops == 2) {
        if (strchr(legal_modes[opcode_num][0], actual_src_mode_char) == NULL) {
            fprintf(stderr, "Error at line %d: Illegal addressing mode for source operand of '%s'.\n", line, opcode);
            success = 0;
        }
    }

    /* Validate destination operand addressing mode */
    if (num_ops == 2) {
        if (strchr(legal_modes[opcode_num][1], actual_dest_mode_char) == NULL) {
            fprintf(stderr, "Error at line %d: Illegal addressing mode for destination operand of '%s'.\n", line, opcode);
            success = 0;
        }
    } else if (num_ops == 1) {
        /* For single operand, it's treated as destination */
        if (strchr(legal_modes[opcode_num][1], actual_src_mode_char) == NULL) {
            fprintf(stderr, "Error at line %d: Illegal addressing mode for operand of '%s'.\n", line, opcode);
            success = 0;
        }
    }

    if (!success) 
        g_has_error = 1;
    return success;
}

/**
 * Main first pass function - processes the entire input file
 * Builds symbol table, validates syntax, creates instruction and data lists
 * @param input Input file pointer
 * @param symTab Pointer to symbol table head
 * @param instructionList Pointer to instruction list head
 * @param dataList Pointer to data list head
 * @param final_ic_out Output for final instruction counter
 * @param final_dc_out Output for final data counter
 * @return 1 on success, 0 if errors occurred
 */
int firstPass(FILE *input, Symbol **symTab, Instruction **instructionList, DataItem **dataList, int *final_ic_out, int *final_dc_out) {
    /* All variable declarations at top for C90 compliance */
    char line[MAX_LINE_LENGTH + 2];  /* +2 for newline and null */
    int lineNumber = 0;
    int IC = MEMORY_START, DC = 0;  /* Instruction and Data counters */
    int c;
    char *p;  /* Line parsing pointer */
    char label_name[MAX_SYMBOL_LENGTH];
    char *colon_pos;
    char command_or_directive[MAX_SYMBOL_LENGTH];
    int chars_read;
    char extern_label_name[MAX_SYMBOL_LENGTH];
    char entry_label_name[MAX_SYMBOL_LENGTH];
    Symbol *s;
    char op1_str[MAX_LINE_LENGTH], op2_str[MAX_LINE_LENGTH];
    int num_ops_found;
    char* rest_of_line;
    char* tokenizer_state;
    char* token;
    Instruction *newInst;
    int k;
    /* Temporary lists built in reverse, then reversed at end */
    Instruction *temp_i_head = NULL;
    DataItem *temp_d_head = NULL;
    /* For list reversal */
    Instruction *prev_i = NULL, *curr_i = NULL, *next_i = NULL;
    DataItem *prev_d = NULL, *curr_d = NULL, *next_d = NULL;

    /* Initialize global error flag */
    g_has_error = 0;

    /* Process input line by line */
    while (fgets(line, sizeof(line), input) != NULL) {
        lineNumber++;
        
        /* Initialize for this line */
        label_name[0] = '\0';
        op1_str[0] = '\0';
        op2_str[0] = '\0';
        num_ops_found = 0;

        /* Check line length limit */
        if (strlen(line) > MAX_LINE_LENGTH && line[MAX_LINE_LENGTH] != '\n' && line[MAX_LINE_LENGTH] != '\0') {
            fprintf(stderr, "Error at line %d: Line exceeds maximum length of %d characters.\n", lineNumber, MAX_LINE_LENGTH);
            g_has_error = 1;
            /* Skip rest of oversized line */
            while ((c = fgetc(input)) != '\n' && c != EOF);
            continue;
        }

        /* Remove newline characters */
        line[strcspn(line, "\r\n")] = '\0';
        
        /* Skip leading whitespace */
        p = skip_whitespace(line);

        /* Skip empty lines and comments */
        if (*p == '\0' || *p == ';') 
            continue;

        /* Check for label definition (ends with colon) */
        colon_pos = strchr(p, ':');
        if (colon_pos) {
            size_t label_len = colon_pos - p;
            
            /* Validate label */
            if (label_len == 0) {
                fprintf(stderr, "Error at line %d: Empty label definition.\n", lineNumber);
                g_has_error = 1; 
                continue;
            }
            if (label_len >= MAX_SYMBOL_LENGTH) {
                fprintf(stderr, "Error at line %d: Label name '%.*s' exceeds max length %d.\n", 
                        lineNumber, (int)label_len, p, MAX_SYMBOL_LENGTH - 1);
                g_has_error = 1; 
                continue;
            }
            
            /* Extract label name */
            strncpy(label_name, p, label_len);
            label_name[label_len] = '\0';
            
            /* Move past the label */
            p = skip_whitespace(colon_pos + 1);
        }

        /* Parse command/directive */
        if (sscanf(p, "%s%n", command_or_directive, &chars_read) != 1) {
            if (label_name[0] != '\0') {
                fprintf(stderr, "Error at line %d: Missing command/directive after label '%s'.\n", lineNumber, label_name);
                g_has_error = 1;
            }
            continue;
        }
        p = skip_whitespace(p + chars_read);

        /* Process directives (start with '.') */
        if (command_or_directive[0] == '.') {
            /* Add label for data directives */
            if (label_name[0]) {
                addSymbol(symTab, label_name, DC, SYMBOL_DATA, lineNumber);
                if (g_has_error) continue;
            }

            /* Process each directive type */
            if (strcmp(command_or_directive, ".data")==0) {
                if (!validate_data_parameters(p, lineNumber, &temp_d_head, &DC)) 
                    continue;
            } else if (strcmp(command_or_directive, ".string")==0) {
                if (!validate_string_parameters(p, lineNumber, &temp_d_head, &DC)) 
                    continue;
            } else if (strcmp(command_or_directive, ".mat")==0) {
                if (!validate_mat_parameters(p, lineNumber, &temp_d_head, &DC)) 
                    continue;
            } else if (strcmp(command_or_directive, ".extern")==0) {
                /* Label on .extern line is ignored */
                if (label_name[0]) {
                    fprintf(stderr, "Warning at line %d: Label '%s' on .extern directive is ignored.\n", lineNumber, label_name);
                }
                /* Extract and add external symbol */
                if(sscanf(p, "%s", extern_label_name) == 1) {
                    addSymbol(symTab, extern_label_name, 0, SYMBOL_EXTERNAL, lineNumber);
                } else {
                    fprintf(stderr, "Error at line %d: Missing label for .extern directive.\n", lineNumber); 
                    g_has_error = 1;
                }
                if (g_has_error) continue;
            } else if (strcmp(command_or_directive, ".entry")==0) {
                /* Label on .entry line is ignored */
                if (label_name[0]) {
                    fprintf(stderr, "Warning at line %d: Label '%s' on .entry directive is ignored.\n", lineNumber, label_name);
                }
                /* Process entry point */
                if(sscanf(p, "%s", entry_label_name) == 1) {
                    s = findSymbol(*symTab, entry_label_name);
                    if (s) {
                        /* Check for conflict with external */
                        if (s->type == SYMBOL_EXTERNAL) {
                            fprintf(stderr, "Error at line %d: Symbol '%s' declared as .entry and .extern.\n", 
                                    lineNumber, entry_label_name); 
                            g_has_error = 1;
                        } else {
                            s->type = SYMBOL_ENTRY; 
                        }
                    } else {
                        /* Add as entry (will be resolved in second pass) */
                        addSymbol(symTab, entry_label_name, 0, SYMBOL_ENTRY, lineNumber);
                    }
                } else {
                    fprintf(stderr, "Error at line %d: Missing label for .entry directive.\n", lineNumber); 
                    g_has_error = 1;
                }
                if (g_has_error) continue;
            } else {
                fprintf(stderr, "Error at line %d: Unrecognized directive '%s'.\n", lineNumber, command_or_directive); 
                g_has_error = 1;
            }
        } else {
            /* Process instruction */
            
            /* Add label for code if present */
            if (label_name[0]) {
                addSymbol(symTab, label_name, IC, SYMBOL_CODE, lineNumber);
                if (g_has_error) continue;
            }
            
            /* Validate opcode */
            if (!is_opcode(command_or_directive)) {
                fprintf(stderr, "Error at line %d: Unrecognized instruction '%s'.\n", lineNumber, command_or_directive); 
                g_has_error = 1; 
                continue;
            }

            /* Parse operands (comma-separated) */
            rest_of_line = p;
            tokenizer_state = rest_of_line;
            
            /* First operand */
            token = strtok_r(tokenizer_state, ",", &tokenizer_state);
            if (token) {
                char* trimmed_op1 = skip_whitespace(token);
                trimmed_op1[strcspn(trimmed_op1, " \t")] = '\0';

                if (strlen(trimmed_op1) > 0) {
                    strncpy(op1_str, trimmed_op1, sizeof(op1_str) - 1);
                    op1_str[sizeof(op1_str) - 1] = '\0';
                    num_ops_found = 1;
                } else {
                    fprintf(stderr, "Error at line %d: Missing first operand or invalid comma usage.\n", lineNumber);
                    g_has_error = 1; 
                    continue;
                }
            }

            /* Second operand */
            token = strtok_r(NULL, ",", &tokenizer_state);
            if (token) {
                char* trimmed_op2;
                if (num_ops_found == 0) {
                    fprintf(stderr, "Error at line %d: Missing first operand before comma.\n", lineNumber);
                    g_has_error = 1; 
                    continue;
                }
                trimmed_op2 = skip_whitespace(token);
                trimmed_op2[strcspn(trimmed_op2, " \t")] = '\0';

                if (strlen(trimmed_op2) > 0) {
                    strncpy(op2_str, trimmed_op2, sizeof(op2_str) - 1);
                    op2_str[sizeof(op2_str) - 1] = '\0';
                    num_ops_found = 2;
                } else {
                    fprintf(stderr, "Error at line %d: Missing second operand after comma.\n", lineNumber);
                    g_has_error = 1; 
                    continue;
                }
            }
            
            /* Check for extra operands */
            token = strtok_r(NULL, ",", &tokenizer_state);
            if (token && strlen(skip_whitespace(token)) > 0) {
                fprintf(stderr, "Error at line %d: Extraneous text or too many operands.\n", lineNumber);
                g_has_error = 1; 
                continue;
            }

            /* Validate operands for this instruction */
            if (!validate_instruction_operands(command_or_directive, op1_str, op2_str, num_ops_found, lineNumber)) {
                continue;
            }

            /* Create instruction node */
            newInst = malloc(sizeof(Instruction));
            if (!newInst) { 
                fprintf(stderr, "Memory allocation error.\n"); 
                exit(1); 
            }
            
            /* Initialize instruction */
            newInst->address = IC;
            newInst->original_line_number = lineNumber;
            strncpy(newInst->opcode, command_or_directive, sizeof(newInst->opcode) - 1);
            newInst->opcode[sizeof(newInst->opcode) - 1] = '\0';
            newInst->num_operands = num_ops_found;
            strncpy(newInst->operand1, op1_str, sizeof(newInst->operand1) - 1);
            newInst->operand1[sizeof(newInst->operand1) - 1] = '\0';
            strncpy(newInst->operand2, op2_str, sizeof(newInst->operand2) - 1);
            newInst->operand2[sizeof(newInst->operand2) - 1] = '\0';

            /* Initialize machine code fields (filled in second pass) */
            newInst->num_operand_words = 0;
            newInst->machine_code_base4[0] = '\0';
            for(k=0; k<4; ++k) 
                newInst->operand_words_base4[k][0] = '\0';

            /* Calculate instruction length */
            newInst->instruction_length = calculate_instruction_length(newInst->opcode, newInst->operand1, newInst->operand2);
            if (newInst->instruction_length == -1) {
                fprintf(stderr, "Error at line %d: Failed to determine instruction length for '%s'.\n", lineNumber, newInst->opcode);
                g_has_error = 1;
                free(newInst);
                continue;
            }

            /* Add to instruction list (built in reverse) */
            newInst->next = temp_i_head;
            temp_i_head = newInst;
            
            /* Update instruction counter */
            IC += newInst->instruction_length;
        }
    }

    /* Reverse instruction list to get correct order */
    curr_i = temp_i_head;
    while(curr_i) { 
        next_i = curr_i->next; 
        curr_i->next = prev_i; 
        prev_i = curr_i; 
        curr_i = next_i; 
    }
    *instructionList = prev_i;

    /* Reverse data list to get correct order */
    curr_d = temp_d_head;
    while(curr_d) { 
        next_d = curr_d->next; 
        curr_d->next = prev_d; 
        prev_d = curr_d; 
        curr_d = next_d; 
    }
    *dataList = prev_d;

    /* Set final counters */
    *final_ic_out = IC;
    *final_dc_out = DC;
    
    /* Update data symbol addresses (add IC to get final addresses) */
    updateDataSymbolsAddresses(*symTab, IC);

    /* Return success/failure */
    return !g_has_error;
}