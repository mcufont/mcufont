CFLAGS = -O0 -Wall -Werror -Wno-unused-function -ansi
CFLAGS += -ggdb
CFLAGS += -DFONT_NAME=fixed_7x14

render_bmp: render_bmp.c rlefont.c fixed_7x14.c
	$(CC) $(CFLAGS) -o $@ $^
