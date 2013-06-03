#include "rlefont.h"

/* Number of reserved codes before the dictionary entries. */
#define DICT_START 4

/* Find a pointer to the glyph matching a given character by searching
 * through the character ranges. If the character is not found, return
 * pointer to the default glyph.
 */
static const uint8_t *find_glyph(const struct rlefont_s *font,
                                 uint16_t character)
{
   unsigned i, index;
   const struct char_range_s *range;
   for (i = 0; i < font->char_range_count; i++)
   {
       range = &font->char_ranges[i];
       index = character - range->first_char;
       if (character >= range->first_char && index < range->char_count)
       {
           uint16_t offset = range->glyph_offsets[index];
           return &range->glyph_data[offset];
       }
   }

   return font->default_glyph;
}

/* Structure to keep track of coordinates of the next pixel to be written,
 * and also the bounds of the character. */
struct renderstate_r
{
    font_pos_t x_begin;
    font_pos_t x_end;
    font_pos_t x;
    font_pos_t y;
    font_pos_t y_end;
    pixel_callback_t callback;
    void *state;
};

/* Call the callback to write one pixel to screen, and advance to next
 * pixel position. */
static void write_pixel(struct renderstate_r *rstate, uint8_t alpha)
{
    rstate->callback(rstate->x, rstate->y, alpha, rstate->state);
    
    rstate->x++;
    if (rstate->x == rstate->x_end)
    {
        rstate->x = rstate->x_begin;
        rstate->y++;
    }
}

/* Skip the given number of pixels (0 alpha) */
static void skip_pixels(struct renderstate_r *rstate, uint8_t count)
{
    rstate->x += count;
    while (rstate->x >= rstate->x_end)
    {
        rstate->x -= rstate->x_end - rstate->x_begin;
        rstate->y++;
    }
}

/* Decode and write out a RLE-encoded dictionary entry. */
static void write_rle_dictentry(const struct rlefont_s *font,
                                struct renderstate_r *rstate,
                                uint8_t index)
{
    uint16_t offset = font->dictionary_offsets[index];
    uint16_t length = font->dictionary_offsets[index + 1] - offset;
    uint16_t i;
    uint8_t j;
    
    for (i = 0; i < length; i++)
    {
        uint8_t code = font->dictionary_data[offset + i];
        if (code & 0x80)
        {
            code &= 0x7F;
            for (j = 0; j < code; j++)
                write_pixel(rstate, 255);
        }
        else
        {
            skip_pixels(rstate, code);
        }
    }
}

/* Decode and write out a reference codeword */
static void write_ref_codeword(const struct rlefont_s *font,
                                struct renderstate_r *rstate,
                                uint8_t code)
{
    if (code == 0)
    {
        write_pixel(rstate, 0);
    }
    else if (code == 1)
    {
        write_pixel(rstate, 255);
    }
    else if (code == 2)
    {
        rstate->y = rstate->y_end; /* End of glyph */
    }
    else if (code < DICT_START)
    {
        /* Reserved */
    }
    else
    {
        write_rle_dictentry(font, rstate, code - DICT_START);
    }
}

/* Decode and write out a reference encoded dictionary entry. */
static void write_ref_dictentry(const struct rlefont_s *font,
                                struct renderstate_r *rstate,
                                uint8_t index)
{
    uint16_t offset = font->dictionary_offsets[index];
    uint16_t length = font->dictionary_offsets[index + 1] - offset;
    uint16_t i;
    
    for (i = 0; i < length; i++)
    {
        uint8_t code = font->dictionary_data[offset + i];
        write_ref_codeword(font, rstate, code);
    }
}

/* Decode and write out an arbitrary glyph codeword */
static void write_glyph_codeword(const struct rlefont_s *font,
                                struct renderstate_r *rstate,
                                uint8_t code)
{
    if (code >= DICT_START + font->rle_entry_count)
    {
        write_ref_dictentry(font, rstate, code - DICT_START);
    }
    else
    {
        write_ref_codeword(font, rstate, code);
    }
}


font_pos_t render_character(const struct rlefont_s *font,
                            font_pos_t x0, font_pos_t y0,
                            uint16_t character,
                            pixel_callback_t callback,
                            void *state)
{
    const uint8_t *p;
    uint8_t width;
    
    struct renderstate_r rstate;
    rstate.x_begin = x0;
    rstate.x_end = x0 + font->width;
    rstate.x = x0;
    rstate.y = y0;
    rstate.y_end = y0 + font->height;
    rstate.callback = callback;
    rstate.state = state;
    
    p = find_glyph(font, character);
    width = *p++;
    while (rstate.y < rstate.y_end)
    {
        write_glyph_codeword(font, &rstate, *p++);
    }
    
    return width;
}

font_pos_t character_width(const struct rlefont_s *font,
                           uint16_t character)
{
    return *find_glyph(font, character);
}
