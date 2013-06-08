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
static uint16_t strip_spaces(const char *text, uint16_t count, uint16_t *last_char)
{
    uint16_t i = 0, result = 0, tmp = 0;
    
    if (!count)
        count = 0xFFFF;
    
    while (count-- && *text)
    {
        i++;
        tmp = utf8_getchar(&text);
        if (!_isspace(tmp))
            result = i;
    }
    
    if (last_char)
    {
        if (!*text)
            *last_char = 0;
        else
            *last_char = tmp;
    }
    
    return result;
}

/* Count the number of space characters in string */
static uint16_t count_spaces(const char *text, uint16_t count)
{
    uint16_t spaces = 0;
    while (count-- && *text)
    {
        if (_isspace(utf8_getchar(&text)))
            spaces++;
    }
    return spaces;
}

/* Render left-aligned string, left edge at x0. */
static void render_left(const struct rlefont_s *font, int16_t x0, int16_t y0,
                        const char *text, uint16_t count,
                        pixel_callback_t callback, void *state)
{
    int16_t x;
    uint16_t c1 = 0, c2;
    
    x = x0 - font->baseline_x;
    while (count--)
    {
        c2 = utf8_getchar(&text);
        
        if (c2 == '\t')
        {
            x = round_to_tab(font, x0, x);
            c1 = c2;
            continue;
        }
        
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
    int16_t x;
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
    count = strip_spaces(text, count, 0);
    
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

void render_justified(const struct rlefont_s *font, int16_t x0, int16_t y0,
                      int16_t width, const char *text, uint16_t count,
                      pixel_callback_t callback, void *state)
{
    int16_t string_width, adjustment;
    uint16_t last_char;
    uint16_t num_spaces;
    count = strip_spaces(text, count, &last_char);
    
    if (last_char == '\n' || last_char == 0)
    {
        /* Line ends in linefeed, do not justify. */
        render_left(font, x0, y0, text, count, callback, state);
        return;
    }
    
    string_width = get_string_width(font, text, count, false);
    adjustment = width - string_width;
    num_spaces = count_spaces(text, count);
    
    {
        int16_t x, tmp;
        uint16_t c1 = 0, c2;
        
        x = x0 - font->baseline_x;
        while (count--)
        {
            c2 = utf8_getchar(&text);
            
            if (c2 == '\t')
            {
                tmp = x;
                x = round_to_tab(font, x0, x);
                adjustment -= x - tmp - character_width(font, '\t');
                num_spaces--;
                c1 = c2;
                continue;
            }
            
            if (_isspace(c2))
            {
                tmp = (adjustment + num_spaces / 2) / num_spaces;
                adjustment -= tmp;
                num_spaces--;
                x += tmp;
            }
            
#ifndef NO_KERNING
            if (c1 != 0)
            {
                tmp = compute_kerning(font, c1, c2);
                x += tmp;
                adjustment -= tmp;
            }
#endif
            x += render_character(font, x, y0, c2, callback, state);
            c1 = c2;
        }
    }
}

