CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb

# Directory containing the font files.
FONTDIR = ../../fonts

# Directory containing the decoder source code.
DECODERDIR = ../../decoder

# List of fonts to include
FONTS = ../../fonts/*.c

# List of required decoder components
DECODER = $(DECODERDIR)/mf_rlefont.c \
          $(DECODERDIR)/mf_encoding.c \
          $(DECODERDIR)/mf_kerning.c \
          $(DECODERDIR)/mf_wordwrap.c \
          $(DECODERDIR)/mf_justify.c

render_bmp: render_bmp.c write_bmp.c $(DECODER) $(FONTS)
	$(CC) $(CFLAGS) -I $(FONTDIR) -I $(DECODERDIR) -o $@ $^
