#include <stdint.h>
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


void convertBitEndianness(unsigned char* array, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; ++i) {
        unsigned char originalByte = array[i];
        unsigned char reversedByte = 0;
        int j = 0;
        for (j = 0; j < 8; ++j) {
            reversedByte |= ((originalByte >> j) & 1) << (7 - j);
        }

        array[i] = reversedByte;
    }
}

void write_bmp_1bpp(const char *filename, const uint8_t *data, int width, int height) {
    /* Open the BMP file for writing in binary mode */ 
    FILE *f = fopen(filename, "wb");

    /* BMP header */ 
    fprintf(f, "BM");  /* Signature */ 
    fwrite("\x00\x00\x00\x00", 4, 1, f); /* File size placeholder */ 
    fwrite("\x00\x00", 2, 1, f); /* Reserved 1 */ 
    fwrite("\x00\x00", 2, 1, f); /* Reserved 2 */ 
    fwrite("\x36\x00\x00\x00", 4, 1, f); /* Pixel data offset */ 

    /* DIB header */ 
    fwrite("\x28\x00\x00\x00", 4, 1, f); /* DIB header size */ 
    fwrite(&width, 4, 1, f); /* Image width */ 
    fwrite(&height, 4, 1, f); /* Image height */ 
    fwrite("\x01\x00", 2, 1, f); /* Color planes (1) */ 
    fwrite("\x01\x00", 2, 1, f); /* Bits per pixel (1) */ 
    fwrite("\x00\x00\x00\x00", 4, 1, f); /* Compression method */ 
    fwrite("\x00\x00\x00\x00", 4, 1, f); /* Image size placeholder */ 
    fwrite("\x13\x0B\x00\x00", 4, 1, f); /* Horizontal resolution (2835 pixels/meter) */ 
    fwrite("\x13\x0B\x00\x00", 4, 1, f); /* Vertical resolution (2835 pixels/meter) */ 
    fwrite("\x02\x00\x00\x00", 4, 1, f); /* Colors in palette (2) */ 
    fwrite("\x00\x00\x00\x00", 4, 1, f); /* Important colors (all) */ 
    
    fwrite("\x00\x00\x00\x00", 4, 1, f); /* Color 0 (black) */ 
    fwrite("\xFF\xFF\xFF\x00", 4, 1, f); /* Color 1 (white) */ 

    /* Image data (1 bit per pixel) */ 
    int padding = (4 - ((width / 8) % 4)) % 4; /* Calculate padding */ 
    int i;
    for (i = height - 1; i >= 0; i--) {
        int j;
        for (j = 0; j < width / 8; j++) {
            fputc(data[i * (width / 8) + j], f); /* Write 1 byte of image data */ 
        }
        int k;
        for (k = 0; k < padding; k++) {
            fputc('\x00', f); /* Write padding */ 
        }
    }

    /* Update file size in BMP header */ 
    fseek(f, 2, SEEK_SET);
    uint32_t fileSize = 14 + 40 + 8 + (width / 8 + padding) * height;
    fwrite(&fileSize, 4, 1, f);

    /* Update image size in DIB header */ 
    fseek(f, 34, SEEK_SET);
    uint32_t imageSize = (width / 8 + padding) * height;
    fwrite(&imageSize, 4, 1, f);

    /* Close the BMP file */ 
    fclose(f);
}
