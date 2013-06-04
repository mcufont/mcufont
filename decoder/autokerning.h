/* Automatic kerning for font rendering. This solves the issue where some
 * fonts (especially serif fonts) have too much space between specific
 * character pairs, like WA or L'.
 */

#ifndef _AUTOKERNING_H_
#define _AUTOKERNING_H_

#include "rlefont.h"

/* Compute the kerning adjustment when c1 is followed by c2.
 * 
 * font: Pointer to the font definition.
 * c1: The previous character.
 * c2: The next character to render.
 * 
 * Returns the offset to add to the x position for c2.
 */
int8_t compute_kerning(const struct rlefont_s *font, uint16_t c1, uint16_t c2);

#endif
