# Path to the command-line utility program
MCUFONT = ../encoder/mcufont

# Names of fonts to process
FONTS = DejaVuSans12 DejaVuSans12bw DejaVuSerif16 DejaVuSerif32 \
	fixed_5x8 fixed_7x14 fixed_10x20

# Characters to include in the fonts
CHARS = 0-255 0x2010-0x2015

all: $(FONTS:=.c) fonts.h

clean:
	rm -f $(FONTS:=.c) $(FONTS:=.h) $(FONTS:=.dat)

fonts.h: $(FONTS:=.h)
	echo $(foreach font,$(FONTS),'\n#include "'$(font)'.h"') > $@

%.c %.h: %.dat
	$(MCUFONT) filter $< $(CHARS)
	$(MCUFONT) optimize $< 50
	$(MCUFONT) export $< $(basename $<)

DejaVuSans12.dat: DejaVuSans.ttf
	$(MCUFONT) import_ttf $< 12

DejaVuSans12bw.dat: DejaVuSans.ttf
	$(MCUFONT) import_ttf $< 12 bw
	
DejaVuSerif16.dat: DejaVuSerif.ttf
	$(MCUFONT) import_ttf $< 16

DejaVuSerif32.dat: DejaVuSerif.ttf
	$(MCUFONT) import_ttf $< 32

%.dat: %.bdf
	$(MCUFONT) import_bdf $<
