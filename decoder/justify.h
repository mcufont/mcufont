/* Text alignment and justification algorithm. Supports left, right, center
 * and justify. Supports tab stops and optionally uses kerning. */

#ifndef _JUSTIFY_H_
#define _JUSTIFY_H_

#include "rlefont.h"
#include <stdbool.h>

enum align_t
{
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

/* Get width of a string in pixels.
 *
 * font:   Pointer to the font definition.
 * text:   Pointer to start of the text to measure.
 * count:  Number of characters on the line or 0 to read until end of string.
 * kern:   True to consider kerning (slower).
 */
int16_t get_string_width(const struct rlefont_s *font, const char *text,
                         uint16_t count, bool kern);

/* Render a single line of aligned text.
 *
 * font:     Pointer to the font definition.
 * x0:       Depending on aligned, either left, center or right edge of target.
 * y0:       Upper edge of the target area.
 * align:    Type of alignment.
 * text:     Pointer to start of the text to render.
 * count:    Number of characters on the line or 0 to read until end of string.
 * callback: Callback to pass to render_character().
 * state:    Free variable for use in the callback.
 */
void render_aligned(const struct rlefont_s *font, int16_t x0, int16_t y0,
                    enum align_t align, const char *text, uint16_t count,
                    pixel_callback_t callback, void *state);

/* Render a single line of justified text.
 *
 * font:     Pointer to the font definition.
 * x0:       Left edge of the target area.
 * y0:       Upper edge of the target area.
 * width:    Width of the target area.
 * text:     Pointer to start of the text to render.
 * count:    Number of characters on the line or 0 to read until end of string.
 * callback: Callback to pass to render_character().
 * state:    Free variable for use in the callback.
 */
void render_justified(const struct rlefont_s *font, int16_t x0, int16_t y0,
                      int16_t width, const char *text, uint16_t count,
                      pixel_callback_t callback, void *state);


#endif
