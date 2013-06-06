#include "autokerning.h"

/* Space between characters, percent of glyph width. */
#ifndef KERNING_SPACE_PERCENT
#define KERNING_SPACE_PERCENT 30
#endif

/* Space between characters, pixels. */
#ifndef KERNING_SPACE_PX
#define KERNING_SPACE_PX 3
#endif

/* Maximum kerning adjustment, percent of glyph width. */
#ifndef KERNING_MAX
#define KERNING_MAX 20
#endif

/* Structure for keeping track of the edge of the glyph as it is rendered.
 * Performs an online least-squares linear regression on the glyph edge.
 */
struct kerning_state_s
{
    int16_t min_y;
    int16_t max_y;
    int16_t prev_y; /* Only for right edge */
    int16_t prev_x;
    uint8_t count;
    int32_t sum_x;
    int32_t sum_y;
    int32_t sum_xy;
    int32_t sum_yy;
};

/* Pixel callback for fitting the left edge of a glyph. */
static void fit_leftedge(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                         void *state)
{
    struct kerning_state_s *s = state;
    
    if (alpha > 7 && y > s->prev_y)
    {
        /* First active pixel on a new row, add to sums. */
        s->count++;
        s->sum_x += x;
        s->sum_y += y;
        s->sum_xy += x * y;
        s->sum_yy += y * y;
        s->prev_y = y;
        
        if (y < s->min_y) s->min_y = y;
        if (y > s->max_y) s->max_y = y;
    }
}

/* Pixel callback for fitting the right edge of a glyph. */
static void fit_rightedge(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                         void *state)
{
    struct kerning_state_s *s = state;
    
    if (y > s->prev_y)
    {
        /* New row begins, add the last active pixel of the previous row. */
        if (s->prev_x > 0)
        {
            s->count++;
            s->sum_x += s->prev_x;
            s->sum_y += s->prev_y;
            s->sum_xy += s->prev_x * s->prev_y;
            s->sum_yy += s->prev_y * s->prev_y;
        }
        s->prev_x = -1;
        s->prev_y = y;
    }
    
    if (alpha > 7)
    {
        s->prev_x = x + count;
        if (y < s->min_y) s->min_y = y;
        if (y > s->max_y) s->max_y = y;
    }
}

/* Using linear regression, compute the points where a line fitted to the
 * glyph edge would intercept the top and bottom of the glyph area. */
static void get_marginpos(struct kerning_state_s *s, uint8_t y0, uint8_t y1,
                          int16_t *top, int16_t *bottom)
{
    int32_t divisor, a, b;
    divisor = s->sum_yy - s->sum_y * s->sum_y / s->count;
    
    a = (s->sum_yy / s->count * s->sum_x - s->sum_xy / s->count * s->sum_y) / divisor;
    b = (s->sum_xy - s->sum_x * s->sum_y / s->count) * 256 / divisor;
    
    *top = a + b * y0 / 256;
    *bottom = a + b * y1 / 256;
}

static int16_t min16(int16_t a, int16_t b) { return (a < b) ? a : b; }
static int16_t max16(int16_t a, int16_t b) { return (a > b) ? a : b; }
static int16_t avg16(int16_t a, int16_t b) { return (a + b) / 2; }

int8_t compute_kerning(const struct rlefont_s *font, uint16_t c1, uint16_t c2)
{
    struct kerning_state_s leftedge = {255, 0, -1, -1, 0, 0, 0, 0, 0};
    struct kerning_state_s rightedge = {255, 0, -1, -1, 0, 0, 0, 0, 0};
    uint8_t w1, w2;
    int16_t y0, y1, e1t, e1b, e2t, e2b;
    int16_t normal_space, min_space, adjust, max_adjust;
    
    /* Find the edges of both glyphs. */
    w1 = render_character(font, 0, 0, c1, fit_rightedge, &rightedge);
    w2 = render_character(font, 0, 0, c2, fit_leftedge, &leftedge);
    
    /* Can't kern for empty glyphs, single-row dashes etc. */
    if (leftedge.count <= 1 || rightedge.count <= 1)
        return 0;
    
    /* Consider the area that is average of the two glyphs. */
    y0 = avg16(rightedge.min_y, leftedge.min_y);
    y1 = avg16(rightedge.max_y, leftedge.max_y);
    
    /* Find the intersection points of the edges. */
    get_marginpos(&rightedge, y0, y1, &e1t, &e1b);
    get_marginpos(&leftedge, y0, y1, &e2t, &e2b);
    
    /* Compute the amount of space available at top & bottom of the glyph. */
    normal_space = avg16(w1, w2) * KERNING_SPACE_PERCENT / 100 + KERNING_SPACE_PX;
    min_space = min16(w1 - e1t + e2t, w1 - e1b + e2b);
    adjust = normal_space - min_space;
    max_adjust = -max16(w1, w2) * KERNING_MAX / 100;
    
    if (adjust > 0) adjust = 0;
    if (adjust < max_adjust) adjust = max_adjust;
    
    return adjust;
}
