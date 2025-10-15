#include "convertToBase4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORD_SIZE 10    /* 10 bits per word in our machine */
#define BASE4_LENGTH 5  /* 10 bits = 5 base-4 digits */

/**
 * Converts a 10-bit value to a base-4 representation.
 * The function ensures a fixed length of 5 digits, padding with 'a' (zero) if necessary.
 * It handles negative numbers using 10-bit two's complement.
 *
 * @param value The integer value to convert.
 * @return A dynamically allocated string with the base-4 representation (always 5 characters).
 */
char* convertToBase4(int value) {
    char temp[BASE4_LENGTH + 1];
    char *result;
    int i;
    
    /* Mask to 10 bits to handle both positive and negative numbers correctly */
    value &= 0x3FF;  /* 0x3FF = 1111111111 in binary (10 bits) */
    
    /* Convert from right to left (least significant digit first) */
    for (i = BASE4_LENGTH - 1; i >= 0; i--) {
        int digit = value & 0x3;  /* Get the last 2 bits */
        
        /* Convert 2-bit value to base-4 character */
        switch(digit) {
            case 0: temp[i] = 'a'; break;
            case 1: temp[i] = 'b'; break;
            case 2: temp[i] = 'c'; break;
            case 3: temp[i] = 'd'; break;
        }
        
        value >>= 2;  /* Shift right by 2 bits for the next digit */
    }
    
    temp[BASE4_LENGTH] = '\0';  /* Null-terminate the temporary string */
    
    /* Allocate memory for the final result and copy the temporary string. */
    /* The spec requires a fixed-width output (5 digits for 10 bits), so we don't remove leading 'a's. */
    result = malloc(BASE4_LENGTH + 1);
    if (!result) {
        fprintf(stderr, "Memory allocation error in convertToBase4\n");
        exit(1);
    }
    
    strcpy(result, temp);
    
    return result;
}

/**
 * @brief Strips leading 'a' characters from a base-4 string.
 * 
 * This function creates a new string without leading 'a's (zeros).
 * If the string is all 'a's, it returns a single 'a'.
 * Used primarily for formatting the header in the object file.
 * 
 * @param base4_str The base-4 string to strip
 * @return A dynamically allocated string without leading 'a's.
 *         The caller must free this memory.
 */
char* stripLeadingA(const char* base4_str) {
    const char* start = base4_str;
    char* result;
    size_t len;
    
    if (!base4_str) return NULL;
    
    /* Skip leading 'a' characters, but keep at least one */
    while (*start == 'a' && *(start + 1) != '\0') {
        start++;
    }
    
    /* Calculate length of result */
    len = strlen(start);
    
    /* Allocate memory for result */
    result = (char*)malloc((len + 1) * sizeof(char));
    if (!result) {
        fprintf(stderr, "Memory allocation error in stripLeadingA\n");
        return NULL;
    }
    
    /* Copy the stripped string */
    strcpy(result, start);
    
    return result;
}