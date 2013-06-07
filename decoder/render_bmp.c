/* Example application that renders the given (UTF-8 or ASCII) string into
 * a BMP image. */

#include "rlefont.h"
#include "fonts.h"
#include "mini_utf8.h"
#include "autokerning.h"
#include "wordwrap.h"
#include <stdio.h>
#include <stdlib.h>

/* Writes a little-endian 16-bit word to the file. */
static void write_16b(FILE *f, uint16_t word)
{
    uint8_t buf[4];
    buf[0] = (word >> 0) & 0xFF;
    buf[1] = (word >> 8) & 0xFF;
    fwrite(buf, 2, 1, f);
}

/* Writes a little-endian 32-bit word to the file. */
static void write_32b(FILE *f, uint32_t word)
{
    uint8_t buf[4];
    buf[0] = (word >> 0) & 0xFF;
    buf[1] = (word >> 8) & 0xFF;
    buf[2] = (word >> 16) & 0xFF;
    buf[3] = (word >> 24) & 0xFF;
    fwrite(buf, 4, 1, f);
}

/* Writes a BMP file. The data is assumed to be 8-bit grayscale. */
static void write_bmp(const char *filename, const uint8_t *data,
                      int width, int height)
{
    int i;
    FILE *f = fopen(filename, "w");
    
    /* Bitmap file header */
    fwrite("BM", 2, 1, f);
    write_32b(f, 14 + 12 + 256 * 3 + width * height); /* File length */
    write_16b(f, 0); /* Reserved */
    write_16b(f, 0); /* Reserved */
    write_32b(f, 14 + 12 + 256 * 3); /* Offset to pixel data */
    
    /* DIB header */
    write_32b(f, 12); /* Header length */
    write_16b(f, width); /* Bitmap width */
    write_16b(f, height); /* Bitmap height */
    write_16b(f, 1); /* Number of planes */
    write_16b(f, 8); /* Bits per pixel */
    
    /* Color table */
    for (i = 0; i < 256; i++)
    {
        uint8_t buf[3];
        buf[0] = buf[1] = buf[2] = i;
        fwrite(buf, 3, 1, f);
    }
    
    /* Pixel data */
    for (i = 0; i < height; i++)
    {
        fwrite(data + width * (height - 1 - i), width, 1, f);
    }
    
    fclose(f);
}

typedef struct {
    uint8_t *buffer;
    uint16_t width;
    uint16_t height;
    uint16_t y;
    const struct rlefont_s *font;
} state_t;

/* Callback to write to a memory buffer. */
void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void *state)
{
    state_t *s = (state_t*)state;
    uint16_t val;
    if (y < 0 || y >= s->height) return;
    
    while (count--)
    {
        if (x < 0 || x >= s->width) return;
    
        val = s->buffer[s->width * y + x] + alpha;
        if (val > 255) val = 255;
        s->buffer[s->width * y + x] = val;
        
        x++;
    }
}

/* Callback to render lines. */
bool line_callback(const char *line, uint16_t length, void *state)
{
    state_t *s = (state_t*)state;
    int x;
    uint16_t c1 = 0, c2;
    
    x = - s->font->baseline_x;
    while (length--)
    {
        c2 = utf8_getchar(&line);
        if (c1 != 0)
            x += compute_kerning(s->font, c1, c2);
        x += render_character(s->font, x, s->y, c2, pixel_callback, state);
        c1 = c2;
    }
    
    s->y += s->font->height;
    return true;
}

/* Callback to just count the lines. */
bool count_lines(const char *line, uint16_t length, void *state)
{
    int *count = (int*)state;
    (*count)++;
    return true;
}

int main(int argc, char **argv)
{
    const char *filename, *string;
    int width = 640, height;
    const struct rlefont_s *font;
    state_t state = {};
    
    if (argc != 4)
    {
        printf("Usage: ./render_bmp font file.bmp string\n");
        return 1;
    }
    
    font = find_font(argv[1], INCLUDED_FONTS);
    
    if (!font)
    {
        printf("No such font: %s\n", argv[1]);
        return 2;
    }
    
    filename = argv[2];
    string = argv[3];
    
    /* Count the number of lines that we need. */
    height = 0;
    wordwrap(font, width, string, count_lines, &height);
    height *= font->height;
    
    /* Allocate and clear the image buffer */
    state.width = width;
    state.height = height;
    state.buffer = calloc(width * height, 1);
    state.y = 0;
    state.font = font;
    
    /* Render the text */
    wordwrap(font, width, string, line_callback, &state);
    
    /* Write out the bitmap */
    write_bmp(filename, state.buffer, width, height);
    
    free(state.buffer);
    return 0;
}

