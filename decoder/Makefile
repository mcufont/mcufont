CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb

FONTS = ../fonts/*.c

render_bmp: render_bmp.c rlefont.c mini_utf8.c autokerning.c $(FONTS) | fonts.h
	$(CC) $(CFLAGS) -I ../fonts -I . -o $@ $^

fonts.h: ../fonts/*.h
	cat ../fonts/*.h > fonts.h
