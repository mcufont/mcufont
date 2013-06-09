CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb

# Directory containing the font files.
FONTDIR = ../../fonts

# Directory containing the decoder source code.
DECODERDIR = ../../decoder

# List of fonts to include
FONTS = ../../fonts/*.c

# List of required decoder components
DECODER = $(DECODERDIR)/rlefont.c \
          $(DECODERDIR)/mini_utf8.c \
          $(DECODERDIR)/autokerning.c \
          $(DECODERDIR)/wordwrap.c \
          $(DECODERDIR)/justify.c

render_bmp: render_bmp.c $(DECODER) $(FONTS)
	$(CC) $(CFLAGS) -I $(FONTDIR) -I $(DECODERDIR) -o $@ $^
