/* Minimal UTF-8 implementation. */

#ifndef _MINI_UTF8_H_
#define _MINI_UTF8_H_

#include <stdint.h>
#include <stdbool.h>

/* Returns the next character in the string and advances the pointer.
 * When the string ends, returns 0 and leaves the pointer at the 0 byte.
 * 
 * str: Pointer to variable holding current location in string.
 *      Initialize it to the start of the string.
 * 
 * Returns: The next character, encoded as UCS-2.
 */
uint16_t utf8_getchar(const char **str);

/* Moves back the pointer to the beginning of the previous character.
 * Be careful not to go beyond the start of the string.
 */
void utf8_rewind(const char **str);

#endif
