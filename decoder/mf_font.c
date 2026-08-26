#include "mf_font.h"
#include <stdbool.h>

/* This will be made into a list of included fonts using macro magic. */
#define MF_INCLUDED_FONTS 0

/* Included fonts begin here */
#include MF_FONT_FILE_NAME
/* Include fonts end here */

uint16_t mf_render_character(const struct mf_font_s *font,
                             int16_t x0, int16_t y0,
                             mf_char character,
                             mf_pixel_callback_t callback,
                             void *state)
{
    uint16_t width;
    width = font->render_character(font, x0, y0, character, callback, state);

    if (!width)
    {
        width = font->render_character(font, x0, y0, font->fallback_character,
                                       callback, state);
    }

    return width;
}

uint16_t mf_character_width(const struct mf_font_s *font,
                            mf_char character)
{
    uint16_t width;
    width = font->character_width(font, character);

    if (!width)
    {
        width = font->character_width(font, font->fallback_character);
    }

    return width;
}

struct whitespace_state
{
    uint16_t min_x, min_y;
    uint16_t max_x, max_y;
};

static void whitespace_callback(int16_t x, int16_t y, uint8_t count,
                                uint8_t alpha, void *state)
{
    struct whitespace_state *s = state;
    if (alpha > 7)
    {
        if (s->min_x > x) s->min_x = x;
        if (s->min_y > y) s->min_y = y;
        x += count - 1;
        if (s->max_x < x) s->max_x = x;
        if (s->max_y < y) s->max_y = y;
    }
}

MF_EXTERN void mf_character_whitespace(const struct mf_font_s *font,
                                       mf_char character,
                                       uint16_t *left, uint16_t *top,
                                       uint16_t *right, uint16_t *bottom)
{
    struct whitespace_state state = {UINT16_MAX, UINT16_MAX, 0, 0};
    mf_render_character(font, 0, 0, character, whitespace_callback, &state);

    if (state.min_x == UINT16_MAX && state.min_y == UINT16_MAX)
    {
        /* Character is whitespace */
        if (left) *left = font->width;
        if (top) *top = font->height;
        if (right) *right = 0;
        if (bottom) *bottom = 0;
    }
    else
    {
        if (left) *left = state.min_x;
        if (top) *top = state.min_y;
        if (right) *right = font->width - state.max_x - 1;
        if (bottom) *bottom = font->height - state.max_y - 1;
    }
}

/* Avoids a dependency on libc */
static bool strequals(const char *a, const char *b)
{
    while (*a)
    {
        if (*a++ != *b++)
            return false;
    }
    return (!*b);
}

const struct mf_font_s *mf_find_font(const char *name)
{
    const struct mf_font_list_s *f;
    f = MF_INCLUDED_FONTS;

    while (f)
    {
        if (strequals(f->font->full_name, name) ||
            strequals(f->font->short_name, name))
        {
            return f->font;
        }

        f = f->next;
    }

    return 0;
}

const struct mf_font_list_s *mf_get_font_list(void)
{
    return MF_INCLUDED_FONTS;
}

