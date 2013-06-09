/* Small internal utility functions */

#ifndef _MF_INTERNAL_H_
#define _MF_INTERNAL_H_

#include "mf_rlefont.h"

static int16_t mf_round_to_tab(const struct mf_rlefont_s *font,
                               int16_t x0, int16_t x)
{
    int16_t tabw, dx;
    tabw = mf_character_width(font, ' ') * MF_TABSIZE;
    
    dx = x - x0 + font->baseline_x;
    x += tabw - (dx % tabw);
    
    return x;
}

/* To avoid dependency on ctype.h */
static bool mf_isspace(uint16_t c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

#endif
