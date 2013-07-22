# Path to the command-line utility program
MCUFONT = ../encoder/mcufont

# Names of fonts to process
FONTS = DejaVuSans12 DejaVuSans12bw DejaVuSerif16 DejaVuSerif32 \
	fixed_5x8 fixed_7x14 fixed_10x20 DejaVuSans12bw_bwfont

# Characters to include in the fonts
CHARS = 0-255 0x2010-0x2015

all: $(FONTS:=.c) $(FONTS:=.dat) fonts.h

clean:
	rm -f $(FONTS:=.c) $(FONTS:=.dat)

fonts.h: $(FONTS:=.c)
	echo $(foreach font,$(FONTS),'\n#include "'$(font)'.c"') > $@

%.c: %.dat $(MCUFONT)
	$(MCUFONT) rlefont_export $<

fixed_5x8.c: fixed_5x8.dat $(MCUFONT)
	$(MCUFONT) bwfont_export $<

DejaVuSans12bw_bwfont.c: DejaVuSans12bw_bwfont.dat $(MCUFONT)
	$(MCUFONT) bwfont_export $<

DejaVuSans12bw_bwfont.dat: DejaVuSans12bw.dat
	cp $< $@
	
DejaVuSans12.dat: DejaVuSans.ttf
	$(MCUFONT) import_ttf $< 12
	$(MCUFONT) filter $@ $(CHARS)
	$(MCUFONT) rlefont_optimize $@ 50

DejaVuSans12bw.dat: DejaVuSans.ttf
	$(MCUFONT) import_ttf $< 12 bw
	$(MCUFONT) filter $@ $(CHARS)
	$(MCUFONT) rlefont_optimize $@ 50

DejaVuSerif16.dat: DejaVuSerif.ttf
	$(MCUFONT) import_ttf $< 16
	$(MCUFONT) filter $@ $(CHARS)
	$(MCUFONT) rlefont_optimize $@ 50

DejaVuSerif32.dat: DejaVuSerif.ttf
	$(MCUFONT) import_ttf $< 32
	$(MCUFONT) filter $@ $(CHARS)
	$(MCUFONT) rlefont_optimize $@ 50

%.dat: %.bdf
	$(MCUFONT) import_bdf $<
	$(MCUFONT) filter $@ $(CHARS)
	$(MCUFONT) rlefont_optimize $@ 50
