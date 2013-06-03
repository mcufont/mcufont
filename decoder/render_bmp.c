/* Example application that renders the given (UTF-8 or ASCII) string into
 * a BMP image. */

#define FONT_STRUCT rlefont_fixed_7x14

#include "rlefont.h"
#include "fixed_7x14.h"
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

void pixel_callback(font_pos_t x, font_pos_t y, uint8_t alpha, void *state)
{
    state_t *s = (state_t*)state;
    uint16_t val;
    
    val = s->buffer[s->width * y + x] + alpha;
    if (val > 255) val = 255;
    s->buffer[s->width * y + x] = val;
}

int main(int argc, char **argv)
{
    const char *filename, *string;
    int width = 0, height = 0, x;
    const char *p;
    
    if (argc != 3)
    {
        printf("Usage: ./render_bmp file.bmp string\n");
        return 1;
    }
    
    filename = argv[1];
    string = argv[2];
    
    /* Figure out the width of the string */
    p = string;
    while (*p)
    {
        width += character_width(&FONT_STRUCT, *p++);
    }
    
    while (width % 4 != 0) width++;
    height = FONT_STRUCT.height;
    
    printf("Bitmap size: %d x %d\n", width, height);
    
    /* Allocate and clear the image buffer */
    state_t state;
    state.width = width;
    state.height = height;
    state.buffer = calloc(width * height, 1);
    
    /* Render the text */
    p = string;
    x = 0;
    while (*p)
    {
        x += render_character(&FONT_STRUCT, x, 0, *p++, pixel_callback, &state);
    }
    
    /* Write out the bitmap */
    write_bmp(filename, state.buffer, width, height);
    
    free(state.buffer);
    return 0;
}

