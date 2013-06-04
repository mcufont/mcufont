/* Example application that renders the given (UTF-8 or ASCII) string into
 * a BMP image. */

#include "rlefont.h"
#include "fonts.h"
#include "mini_utf8.h"
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
} state_t;

/* Callback to write to a memory buffer. */
void pixel_callback(int16_t x, int16_t y, uint8_t alpha, void *state)
{
    state_t *s = (state_t*)state;
    uint16_t val;
    
    if (x < 0 || x >= s->width) return;
    if (y < 0 || y >= s->height) return;
    
    val = s->buffer[s->width * y + x] + alpha;
    if (val > 255) val = 255;
    s->buffer[s->width * y + x] = val;
}

int main(int argc, char **argv)
{
    const char *filename, *string;
    int width = 0, height = 0, x;
    const char *p;
    const struct rlefont_s *font;
    
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
    
    /* Figure out the width of the string */
    p = string;
    while (*p)
    {
        width += character_width(font, utf8_getchar(&p));
    }
    
    while (width % 4 != 0) width++;
    height = font->height;
    
    printf("Bitmap size: %d x %d\n", width, height);
    
    /* Allocate and clear the image buffer */
    state_t state;
    state.width = width;
    state.height = height;
    state.buffer = calloc(width * height, 1);
    
    /* Render the text */
    p = string;
    x = - font->baseline_x;
    while (*p)
    {
        x += render_character(font, x, 0, utf8_getchar(&p), pixel_callback, &state);
    }
    
    /* Write out the bitmap */
    write_bmp(filename, state.buffer, width, height);
    
    free(state.buffer);
    return 0;
}

