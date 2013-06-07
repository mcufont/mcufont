/* Word wrapping algorithm with UTF-8 support. More than just a basic greedy
 * word-wrapper: it attempts to balance consecutive lines as pairs.
 */

#ifndef _WORDWRAP_H_
#define _WORDWRAP_H_

#include "rlefont.h"
#include "fontutils.h"
#include "mini_utf8.h"
#include <stdbool.h>

/* Callback function for handling each line.
 *
 * begin:   Pointer to the beginning of the string for this line.
 * count:  Number of characters on the line.
 * state:   Free variable that was passed to wordwrap().
 * 
 * Returns: true to continue, false to stop after this line.
 */
typedef bool (*line_callback_t) (const char *line, uint16_t count,
                                 void *state);

/* Word wrap a piece of text. Calls the callback function for each line.
 * 
 * font:  Font to use for metrics.
 * width: Maximum line width in pixels.
 * text:  Pointer to the start of the text to process.
 * state: Free variable for caller to use (can be NULL).
 */
void wordwrap(const struct rlefont_s *font, int16_t width,
              const char *text, line_callback_t callback, void *state);
              
#endif
