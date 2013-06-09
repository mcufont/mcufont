/* Type definitions and decoding functions for a font compression format using
 * run length encoding and dictionary compression. Font files are written
 * out as C source code by a generator program.
 */

#ifndef _MF_RLEFONT_H_
#define _MF_RLEFONT_H_

#include "mf_encoding.h"

/* Structure for a range of characters. This implements a sparse storage of
 * character indices, so that you can e.g. pick a 100 characters in the middle
 * of the UTF16 range and just store them. */
struct mf_char_range_s
{
    /* The number of the first character in this range. */
    uint16_t first_char;
    
    /* The total count of characters in this range. */
    uint16_t char_count;
    
    /* Lookup table with the start indices into glyph_data. */
    const uint16_t *glyph_offsets;
    
    /* The encoded glyph data for glyphs in this range. */
    const uint8_t *glyph_data;
};

/* Structure for a single encoded font. */
struct mf_rlefont_s
{
    /* Full name of the font, comes from the original font file. */
    const char *full_name;
    
    /* Short name of the font, comes from file name. */
    const char *short_name;
    
    /* Big array of the data for all the dictionary entries. */
    const uint8_t *dictionary_data;
    
    /* Lookup table with the start indices into dictionary_data.
     * Contains N+1 entries, so that the length of the entry can
     * be determined by subtracting from the next offset. */
    const uint16_t *dictionary_offsets;
    
    /* Number of dictionary entries using the RLE encoding.
     * Entries starting at this index use the dictionary encoding. */
    const uint8_t rle_entry_count;
    
    /* Total number of dictionary entries.
     * Entries after this are nonexistent. */
    const uint8_t dict_entry_count;
    
    /* Pointer to the default glyph, i.e. glyph to use for missing chars. */
    const uint8_t *default_glyph;
    
    /* Number of discontinuous character ranges */
    const uint8_t char_range_count;
    
    /* Array of the character ranges */
    const struct mf_char_range_s *char_ranges;
    
    /* Width and height of the character bounding box. */
    uint8_t width;
    uint8_t height;
    
    /* Location of the text baseline relative to character. */
    uint8_t baseline_x;
    uint8_t baseline_y;
};

/* Lookup structure for searching fonts by name. */
struct mf_rlefont_list_s
{
    const struct mf_rlefont_list_s *next;
    const struct mf_rlefont_s *font;
};

/* Callback function that writes pixels to screen / buffer / whatever.
 *
 * x:     X coordinate of the first pixel to write.
 * y:     Y coordinate of the first pixel to write.
 * count: Number of pixels to fill (horizontally).
 * alpha: The "opaqueness" of the pixels, 0 for background, 255 for text.
 * state: Free variable that was passed to render_character().
 */
typedef void (*mf_pixel_callback_t) (int16_t x, int16_t y, uint8_t count,
                                     uint8_t alpha, void *state);

/* Function to decode and render a single character. 
 * 
 * font:      Pointer to the font definition.
 * x0, y0:    Upper left corner of the target area.
 * character: The character code (unicode) to render.
 * callback:  Callback function to write out the pixels.
 * state:     Free variable for caller to use (can be NULL).
 * 
 * Returns width of the character.
 */
MF_EXTERN uint8_t mf_render_character(const struct mf_rlefont_s *font,
                                      int16_t x0, int16_t y0,
                                      mf_char character,
                                      mf_pixel_callback_t callback,
                                      void *state);

/* Function to get the width of a single character.
 * This is not necessarily the bounding box of the character
 * data, but rather the tracking width.
 *
 * font:      Pointer to the font definition.
 * character: The character code (unicode) to render.
 * 
 * Returns width of the character in pixels.
 */
MF_EXTERN uint8_t mf_character_width(const struct mf_rlefont_s *font,
                                     mf_char character);

/* Find a font based on name. The name can be either short name or full name.
 * Note: You can pass MF_INCLUDED_FONTS to search among all the included .h
 * files.
 *
 * name: Font name to search for.
 * fonts: Pointer to the first font search entry.
 *
 * Returns a pointer to the font or NULL if not found.
 */
MF_EXTERN const struct mf_rlefont_s *mf_find_font(const char *name,
                                        const struct mf_rlefont_list_s *fonts);

#ifdef __cplusplus
}
#endif

#endif
