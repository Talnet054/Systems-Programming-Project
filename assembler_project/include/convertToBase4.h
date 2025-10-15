#ifndef CONVERTTOBASE4_H
#define CONVERTTOBASE4_H

/**
 * @brief Converts a 10-bit value to a base-4 representation.
 * @param value The integer value to convert.
 * @return A dynamically allocated string with the base-4 representation (always 5 characters).
 */
char* convertToBase4(int value);

/**
 * @brief Strips leading 'a' characters from a base-4 string.
 * @param base4_str The base-4 string to strip
 * @return A dynamically allocated string without leading 'a's.
 */
char* stripLeadingA(const char* base4_str);

#endif /* CONVERTTOBASE4_H */