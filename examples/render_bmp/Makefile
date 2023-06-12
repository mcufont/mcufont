CFLAGS = -O0 -Wall -Werror -ansi
CFLAGS += -ggdb

# Directory containing the font files.
FONTDIR = ../../fonts

# Directory containing the decoder source code.
MFDIR = ../../decoder
include $(MFDIR)/mcufont.mk

all: render_bmp

render_bmp: render_bmp.c write_bmp.c $(MFSRC)
	$(CC) $(CFLAGS) -I $(FONTDIR) -I $(MFINC) -o $@ $^

clean:
	rm -f render_bmp
