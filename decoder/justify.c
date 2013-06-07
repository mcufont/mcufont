#include "justify.h"
#include "mini_utf8.h"
#include "fontutils.h"

#ifndef NO_KERNING
#include "autokerning.h"
#endif

int16_t get_string_width(const struct rlefont_s *font, const char *text,
                         uint16_t count, bool kern)
{
    int16_t result = 0;
    uint16_t c1 = 0, c2;
    
    if (!count)
        count = 0xFFFF;
    
    while (count-- && *text)
    {
        c2 = utf8_getchar(&text);
#ifndef NO_KERNING
        if (kern && c1 != 0)
            result += compute_kerning(font, c1, c2);
#endif
        result += character_width(font, c2);
        c1 = c2;
    }
    
    return result;
}

/* Return the length of the string without trailing spaces. */
static uint16_t strip_spaces(const char *text, uint16_t count)
{
    uint16_t i = 0, result;
    
    if (!count)
        count = 0xFFFF;
    
    while (count-- && *text)
    {
        i++;
        if (!_isspace(utf8_getchar(&text)))
            result = i;
    }
    
    return result;
}

/* Render left-aligned string, left edge at x0. */
static void render_left(const struct rlefont_s *font, int16_t x0, int16_t y0,
                        const char *text, uint16_t count,
                        pixel_callback_t callback, void *state)
{
    int x;
    uint16_t c1 = 0, c2;
    
    x = x0 - font->baseline_x;
    while (count--)
    {
        c2 = utf8_getchar(&text);
#ifndef NO_KERNING
        if (c1 != 0)
            x += compute_kerning(font, c1, c2);
#endif
        x += render_character(font, x, y0, c2, callback, state);
        c1 = c2;
    }
}

/* Render right-aligned string, right edge at x0. */
static void render_right(const struct rlefont_s *font, int16_t x0, int16_t y0,
                        const char *text, uint16_t count,
                        pixel_callback_t callback, void *state)
{
    int x;
    uint16_t i, c1, c2 = 0;
    const char *tmp;
    
    /* Go to the end of the line. */
    for (i = 0; i < count; i++)
        utf8_getchar(&text);
    
    x = x0 - font->baseline_x;
    for (i = 0; i < count; i++)
    {
        utf8_rewind(&text);
        tmp = text;
        c1 = utf8_getchar(&tmp);
        x -= character_width(font, c1);
        
#ifndef NO_KERNING
        if (c2 != 0)
            x -= compute_kerning(font, c1, c2);
#endif
        
        render_character(font, x, y0, c1, callback, state);
        c2 = c1;
    }
}

void render_aligned(const struct rlefont_s *font, int16_t x0, int16_t y0,
                    enum align_t align, const char *text, uint16_t count,
                    pixel_callback_t callback, void *state)
{
    int16_t string_width;
    count = strip_spaces(text, count);
    
    if (align == ALIGN_LEFT)
    {
        render_left(font, x0, y0, text, count, callback, state);
    }
    if (align == ALIGN_CENTER)
    {
        string_width = get_string_width(font, text, count, false);
        x0 -= string_width / 2;
        render_left(font, x0, y0, text, count, callback, state);
    }
    else if (align == ALIGN_RIGHT)
    {
        render_right(font, x0, y0, text, count, callback, state);
    }
}

