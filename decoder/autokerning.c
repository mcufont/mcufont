#include "autokerning.h"

/* Space between characters, percent of glyph width. */
#ifndef KERNING_SPACE_PERCENT
#define KERNING_SPACE_PERCENT 15
#endif

/* Space between characters, pixels. */
#ifndef KERNING_SPACE_PX
#define KERNING_SPACE_PX 3
#endif

/* Maximum kerning adjustment, percent of glyph width. */
#ifndef KERNING_MAX
#define KERNING_MAX 20
#endif

/* Number of kerning zones to divide the glyph height into. */
#ifndef KERNING_ZONES
#define KERNING_ZONES 16
#endif

/* Structure for keeping track of the edge of the glyph as it is rendered. */
struct kerning_state_s
{
    uint8_t edgepos[KERNING_ZONES];
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

int8_t compute_kerning(const struct rlefont_s *font, uint16_t c1, uint16_t c2)
{
    struct kerning_state_s leftedge, rightedge;
    uint8_t w1, w2, i, min_space;
    int16_t normal_space, adjust, max_adjust;
    
    /* Initialize structures */
    i = max16(font->height / KERNING_ZONES, 1);
    leftedge.zoneheight = rightedge.zoneheight = i;
    for (i = 0; i < KERNING_ZONES; i++)
    {
        leftedge.edgepos[i] = 255;
        rightedge.edgepos[i] = 0;
    }
    
    /* Analyze the edges of both glyphs. */
    w1 = render_character(font, 0, 0, c1, fit_rightedge, &rightedge);
    w2 = render_character(font, 0, 0, c2, fit_leftedge, &leftedge);
    
    /* Find the minimum horizontal space between the glyphs. */
    min_space = 255;
    for (i = 0; i < KERNING_ZONES; i++)
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
    normal_space = avg16(w1, w2) * KERNING_SPACE_PERCENT / 100 + KERNING_SPACE_PX;
    adjust = normal_space - min_space;
    max_adjust = -max16(w1, w2) * KERNING_MAX / 100;
    
    if (adjust > 0) adjust = 0;
    if (adjust < max_adjust) adjust = max_adjust;
    
    return adjust;
}
