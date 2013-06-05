#include "rlefont.h"

/* Number of reserved codes before the dictionary entries. */
#define DICT_START 24

/* Special reference to mean "fill with zeros to the end of the glyph" */
#define REF_FILLZEROS 16

/* RLE codes */
#define RLE_CODEMASK    0xC0
#define RLE_VALMASK     0x3F
#define RLE_ZEROS       0x00
#define RLE_64ZEROS     0x40
#define RLE_ONES        0x80
#define RLE_SHADE       0xC0

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
    int16_t x_begin;
    int16_t x_end;
    int16_t x;
    int16_t y;
    int16_t y_end;
    pixel_callback_t callback;
    void *state;
};

/* Call the callback to write one pixel to screen, and advance to next
 * pixel position. */
static void write_pixels(struct renderstate_r *rstate, uint16_t count, uint8_t alpha)
{
    uint8_t rowlen;
    
    /* Write row-by-row if the run spans multiple rows. */
    while (rstate->x + count >= rstate->x_end)
    {
        rowlen = rstate->x_end - rstate->x;
        rstate->callback(rstate->x, rstate->y, rowlen, alpha, rstate->state);
        count -= rowlen;
        rstate->x = rstate->x_begin;
        rstate->y++;
    }
    
    /* Write the remaining part */
    if (count)
    {
        rstate->callback(rstate->x, rstate->y, count, alpha, rstate->state);
        rstate->x += count;
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
    
    for (i = 0; i < length; i++)
    {
        uint8_t code = font->dictionary_data[offset + i];
        if ((code & RLE_CODEMASK) == RLE_ZEROS)
        {
            write_pixels(rstate, code & RLE_VALMASK, 0);
        }
        else if ((code & RLE_CODEMASK) == RLE_64ZEROS)
        {
            write_pixels(rstate, ((code & RLE_VALMASK) + 1) * 64, 0);
        }
        else if ((code & RLE_CODEMASK) == RLE_ONES)
        {
            write_pixels(rstate, (code & RLE_VALMASK) + 1, 255);
        }
        else if ((code & RLE_CODEMASK) == RLE_SHADE)
        {
            uint8_t count, alpha;
            count = (code & RLE_VALMASK) >> 4;
            alpha = ((code & RLE_VALMASK) >> 4) * 0x11;
            write_pixels(rstate, count, alpha);
        }
    }
}

/* Decode and write out a reference codeword */
static void write_ref_codeword(const struct rlefont_s *font,
                                struct renderstate_r *rstate,
                                uint8_t code)
{
    if (code <= 15)
    {
        write_pixels(rstate, 1, 0x10 * code);
    }
    else if (code == 16)
    {
        uint16_t remaining;
        remaining = (rstate->y_end - rstate->y) * font->width
                    - (rstate->x - rstate->x_begin);
        write_pixels(rstate, remaining, 0);
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


uint8_t render_character(const struct rlefont_s *font,
                         int16_t x0, int16_t y0,
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

uint8_t character_width(const struct rlefont_s *font,
                           uint16_t character)
{
    return *find_glyph(font, character);
}

/* Avoids a dependency on libc */
static uint8_t strequals(const char *a, const char *b)
{
    while (*a)
    {
        if (*a++ != *b++)
            return 0;
    }
    return 1;
}

const struct rlefont_s *find_font(const char *name,
                                  const struct rlefont_list_s *fonts)
{
    const struct rlefont_list_s *f;
    f = fonts;
    
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

