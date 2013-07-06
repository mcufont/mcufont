#include "mf_kerning.h"

#if MF_USE_KERNING

/* Structure for keeping track of the edge of the glyph as it is rendered. */
struct kerning_state_s
{
    uint8_t edgepos[MF_KERNING_ZONES];
    uint8_t zoneheight;
};

/* Pixel callback for analyzing the left edge of a glyph. */
static void fit_leftedge(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                         void *state)
{
    struct kerning_state_s *s = state;
    
    if (alpha > 7)
    {
        uint8_t zone = y / s->zoneheight;
        if (x < s->edgepos[zone])
            s->edgepos[zone] = x;
    }
}

/* Pixel callback for analyzing the right edge of a glyph. */
static void fit_rightedge(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                         void *state)
{
    struct kerning_state_s *s = state;
    
    if (alpha > 7)
    {
        uint8_t zone = y / s->zoneheight;
        x += count - 1;
        if (x > s->edgepos[zone])
            s->edgepos[zone] = x;
    }
}

static int16_t min16(int16_t a, int16_t b) { return (a < b) ? a : b; }
static int16_t max16(int16_t a, int16_t b) { return (a > b) ? a : b; }
static int16_t avg16(int16_t a, int16_t b) { return (a + b) / 2; }

int8_t mf_compute_kerning(const struct mf_rlefont_s *font,
                          mf_char c1, mf_char c2)
{
    struct kerning_state_s leftedge, rightedge;
    uint8_t w1, w2, i, min_space;
    int16_t normal_space, adjust, max_adjust;
    
    /* Compute the height of one kerning zone in pixels */
    i = (font->height + MF_KERNING_ZONES - 1) / MF_KERNING_ZONES;
    if (i < 1) i = 1;
    
    /* Initialize structures */
    leftedge.zoneheight = rightedge.zoneheight = i;
    for (i = 0; i < MF_KERNING_ZONES; i++)
    {
        leftedge.edgepos[i] = 255;
        rightedge.edgepos[i] = 0;
    }
    
    /* Analyze the edges of both glyphs. */
    w1 = mf_render_character(font, 0, 0, c1, fit_rightedge, &rightedge);
    w2 = mf_render_character(font, 0, 0, c2, fit_leftedge, &leftedge);
    
    /* Find the minimum horizontal space between the glyphs. */
    min_space = 255;
    for (i = 0; i < MF_KERNING_ZONES; i++)
    {
        uint8_t space;
        if (leftedge.edgepos[i] == 255 || rightedge.edgepos[i] == 0)
            continue; /* Outside glyph area. */
        
        space = w1 - rightedge.edgepos[i] + leftedge.edgepos[i];
        if (space < min_space)
            min_space = space;
    }
    
    if (min_space == 255)
        return 0; /* One of the characters is space, or both are punctuation. */
    
    /* Compute the adjustment of the glyph position. */
    normal_space = avg16(w1, w2) * MF_KERNING_SPACE_PERCENT / 100;
    normal_space += MF_KERNING_SPACE_PIXELS;
    adjust = normal_space - min_space;
    max_adjust = -max16(w1, w2) * MF_KERNING_LIMIT / 100;
    
    if (adjust > 0) adjust = 0;
    if (adjust < max_adjust) adjust = max_adjust;
    
    return adjust;
}

#endif
