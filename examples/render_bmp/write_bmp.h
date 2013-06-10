#ifndef _WRITE_BMP_H_
#define _WRITE_BMP_H_

#include <stdint.h>

/* Writes a BMP file. The data is assumed to be 8-bit grayscale. */
void write_bmp(const char *filename, const uint8_t *data,
               int width, int height);

#endif