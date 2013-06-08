/* Small internal utility functions */

#ifndef _FONTUTILS_H_
#define _FONTUTILS_H_

#include "rlefont.h"

/* The spacing of tab stops, defined by the width of the space character. */
#ifndef TABSIZE
#define TABSIZE 8
#endif

static int16_t round_to_tab(const struct rlefont_s *font, int16_t x0, int16_t x)
{
    int16_t tabw, dx;
    tabw = character_width(font, ' ') * TABSIZE;
    
    dx = x - x0 + font->baseline_x;
    x += tabw - (dx % tabw);
    
    return x;
}

/* To avoid dependency on ctype.h */
static bool _isspace(uint16_t c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

#endif
