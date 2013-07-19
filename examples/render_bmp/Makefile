CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb

# Directory containing the font files.
FONTDIR = ../../fonts

# Directory containing the decoder source code.
DECODERDIR = ../../decoder

# List of fonts to include
FONTS = ../../fonts/*.c

# List of required decoder components
DECODER = $(DECODERDIR)/mf_encoding.c \
          $(DECODERDIR)/mf_font.c \
          $(DECODERDIR)/mf_justify.c \
          $(DECODERDIR)/mf_kerning.c \
          $(DECODERDIR)/mf_rlefont.c \
          $(DECODERDIR)/mf_bwfont.c \
          $(DECODERDIR)/mf_wordwrap.c
          
all: render_bmp

render_bmp: render_bmp.c write_bmp.c $(DECODER) $(FONTS)
	$(CC) $(CFLAGS) -I $(FONTDIR) -I $(DECODERDIR) -o $@ $^

clean:
	rm -f render_bmp
