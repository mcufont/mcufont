#include <stdio.h>
#include "write_bmp.h"

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
void write_bmp(const char *filename, const uint8_t *data,
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
