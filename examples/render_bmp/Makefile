CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb

# Directory containing the font files.
FONTDIR = ../../fonts

# Directory containing the decoder source code.
MFDIR = ../../decoder
include $(MFDIR)/mcufont.mk

# List of fonts to include
FONTS = ../../fonts/*.c
          
all: render_bmp

render_bmp: render_bmp.c write_bmp.c $(MFSRC) $(FONTS)
	$(CC) $(CFLAGS) -I $(FONTDIR) -I $(MFINC) -o $@ $^

clean:
	rm -f render_bmp
