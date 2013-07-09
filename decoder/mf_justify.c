#include "mf_justify.h"
#include "mf_kerning.h"

/* Returns true if the character is a justification point, i.e. expands
 * when the text is being justified. */
static bool is_justify_space(uint16_t c)
{
    return c == ' ' || c == 0xA0;
}

/* Round the X coordinate up to the nearest tab stop. */
static int16_t mf_round_to_tab(const struct mf_font_s *font,
                               int16_t x0, int16_t x)
{
    int16_t spacew, tabw, dx;
    
    spacew = mf_character_width(font, ' ');
    tabw = spacew * MF_TABSIZE;
    
    /* Always atleast 1 space */
    x += spacew;
    
    /* Round to next tab stop */
    dx = x - x0 + font->baseline_x;
    x += tabw - (dx % tabw);
    
    return x;
}

int16_t mf_get_string_width(const struct mf_font_s *font, mf_str text,
                            uint16_t count, bool kern)
{
    int16_t result = 0;
    uint16_t c1 = 0, c2;
    
    if (!count)
        count = 0xFFFF;
    
    while (count-- && *text)
    {
        c2 = mf_getchar(&text);

        if (kern && c1 != 0)
            result += mf_compute_kerning(font, c1, c2);

        result += mf_character_width(font, c2);
        c1 = c2;
    }
    
    return result;
}

/* Return the length of the string without trailing spaces. */
static uint16_t strip_spaces(mf_str text, uint16_t count, mf_char *last_char)
{
    uint16_t i = 0, result = 0;
    mf_char tmp = 0;
    
    if (!count)
        count = 0xFFFF;
    
    while (count-- && *text)
    {
        i++;
        tmp = mf_getchar(&text);
        if (!is_justify_space(tmp) &&
            tmp != '\n' && tmp != '\r' && tmp != '\t')
        {
            result = i;
        }
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

/* Render left-aligned string, left edge at x0. */
static void render_left(const struct mf_font_s *font,
                        int16_t x0, int16_t y0,
                        mf_str text, uint16_t count,
                        mf_pixel_callback_t callback, void *state)
{
    int16_t x;
    mf_char c1 = 0, c2;
    
    x = x0 - font->baseline_x;
    while (count--)
    {
        c2 = mf_getchar(&text);
        
        if (c2 == '\t')
        {
            x = mf_round_to_tab(font, x0, x);
            c1 = c2;
            continue;
        }
        
        if (c1 != 0)
            x += mf_compute_kerning(font, c1, c2);

        x += mf_render_character(font, x, y0, c2, callback, state);
        c1 = c2;
    }
}

/* Render right-aligned string, right edge at x0. */
static void render_right(const struct mf_font_s *font,
                         int16_t x0, int16_t y0,
                         mf_str text, uint16_t count,
                         mf_pixel_callback_t callback, void *state)
{
    int16_t x;
    uint16_t i;
    mf_char c1, c2 = 0;
    mf_str tmp;
    
    /* Go to the end of the line. */
    for (i = 0; i < count; i++)
        mf_getchar(&text);
    
    x = x0 - font->baseline_x;
    for (i = 0; i < count; i++)
    {
        mf_rewind(&text);
        tmp = text;
        c1 = mf_getchar(&tmp);
        x -= mf_character_width(font, c1);
        
        if (c2 != 0)
            x -= mf_compute_kerning(font, c1, c2);
        
        mf_render_character(font, x, y0, c1, callback, state);
        c2 = c1;
    }
}

void mf_render_aligned(const struct mf_font_s *font,
                       int16_t x0, int16_t y0,
                       enum mf_align_t align,
                       mf_str text, uint16_t count,
                       mf_pixel_callback_t callback, void *state)
{
    int16_t string_width;
    count = strip_spaces(text, count, 0);
    
    if (align == MF_ALIGN_LEFT)
    {
        render_left(font, x0, y0, text, count, callback, state);
    }
    if (align == MF_ALIGN_CENTER)
    {
        string_width = mf_get_string_width(font, text, count, false);
        x0 -= string_width / 2;
        render_left(font, x0, y0, text, count, callback, state);
    }
    else if (align == MF_ALIGN_RIGHT)
    {
        render_right(font, x0, y0, text, count, callback, state);
    }
}

/* Count the number of space characters in string */
static uint16_t count_spaces(mf_str text, uint16_t count)
{
    uint16_t spaces = 0;
    while (count-- && *text)
    {
        if (is_justify_space(mf_getchar(&text)))
            spaces++;
    }
    return spaces;
}

void mf_render_justified(const struct mf_font_s *font,
                         int16_t x0, int16_t y0, int16_t width,
                         mf_str text, uint16_t count,
                         mf_pixel_callback_t callback, void *state)
{
    int16_t string_width, adjustment;
    uint16_t num_spaces;
    mf_char last_char;
    
    count = strip_spaces(text, count, &last_char);
    
    if (last_char == '\n' || last_char == 0)
    {
        /* Line ends in linefeed, do not justify. */
        render_left(font, x0, y0, text, count, callback, state);
        return;
    }
    
    string_width = mf_get_string_width(font, text, count, false);
    adjustment = width - string_width;
    num_spaces = count_spaces(text, count);
    
    {
        int16_t x, tmp;
        uint16_t c1 = 0, c2;
        
        x = x0 - font->baseline_x;
        while (count--)
        {
            c2 = mf_getchar(&text);
            
            if (c2 == '\t')
            {
                tmp = x;
                x = mf_round_to_tab(font, x0, x);
                adjustment -= x - tmp - mf_character_width(font, '\t');
                c1 = c2;
                continue;
            }
            
            if (is_justify_space(c2))
            {
                tmp = (adjustment + num_spaces / 2) / num_spaces;
                adjustment -= tmp;
                num_spaces--;
                x += tmp;
            }
            
            if (c1 != 0)
            {
                tmp = mf_compute_kerning(font, c1, c2);
                x += tmp;
                adjustment -= tmp;
            }

            x += mf_render_character(font, x, y0, c2, callback, state);
            c1 = c2;
        }
    }
}

