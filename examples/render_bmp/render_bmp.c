/* Example application that renders the given (UTF-8 or ASCII) string into
 * a BMP image. */

#include <mcufont.h>
#include <fonts.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "write_bmp.h"

/***************************************
 * Parsing of the command line options *
 ***************************************/

typedef struct {
    const char *fontname;
    const char *filename;
    const char *text;
    bool justify;
    enum mf_align_t alignment;
    int width;
    int margin;
    int anchor;
} options_t;

static const char default_text[] = 
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog.\n"
    "0123456789 !?()[]{}/\\+-*";

/* Parse the command line options */
static bool parse_options(int argc, const char **argv, options_t *options)
{
    char align = 'j';
    const char **end = argv + argc;
    
    memset(options, 0, sizeof(options_t));
    
    options->fontname = MF_INCLUDED_FONTS->font->short_name;
    options->filename = "out.bmp";
    options->text = default_text;
    options->width = 200;
    options->margin = 5;
    
    while (argv != end)
    {
        const char *cmd = *argv++;
        if (strcmp(cmd, "-f") == 0 && argc)
        {
            options->fontname = *argv++;
        }
        else if (strcmp(cmd, "-o") == 0 && argc)
        {
            options->filename = *argv++;
        }
        else if (strcmp(cmd, "-a") == 0 && argc)
        {
            align = *(*argv++);
        }
        else if (strcmp(cmd, "-w") == 0 && argc)
        {
            options->width = atoi(*argv++);
        }
        else if (strcmp(cmd, "-m") == 0 && argc)
        {
            options->margin = atoi(*argv++);
        }
        else if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0)
        {
            return false;
        }
        else
        {
            options->text = cmd;
        }
    }
    
    if (!options->text)
    {
        printf("The text to render is missing.\n");
        return false;
    }
    
    if (align == 'l')
    {
        options->alignment = MF_ALIGN_LEFT;
        options->anchor = options->margin;
    }
    else if (align == 'c')
    {
        options->alignment = MF_ALIGN_CENTER;
        options->anchor = options->width / 2;
    }
    else if (align == 'r')
    {
        options->alignment = MF_ALIGN_RIGHT;
        options->anchor = options->width - options->margin;
    }
    else if (align == 'j')
    {
        options->justify = true;
        options->anchor = options->margin;
    }
    else
    {
        printf("Invalid alignment: %c\n", align);
        return false;
    }
    
    if (options->width <= 0)
    {
        printf("Invalid width: %d\n", options->width);
        return false;
    }
    
    /* Round to a multiple of 4 pixels */
    if (options->width % 4 != 0)
        options->width += 4 - options->width % 4;
    
    return true;
}

/********************************************
 * Callbacks to specify rendering behaviour *
 ********************************************/

typedef struct {
    options_t *options;
    uint8_t *buffer;
    uint16_t width;
    uint16_t height;
    uint16_t y;
    const struct mf_font_s *font;
} state_t;

/* Callback to write to a memory buffer. */
static void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                           void *state)
{
    state_t *s = (state_t*)state;
    uint32_t pos;
    int16_t value;
    
    if (y < 0 || y >= s->height) return;
    if (x < 0 || x + count >= s->width) return;
    
    while (count--)
    {
        pos = (uint32_t)s->width * y + x;
        value = s->buffer[pos];
        value -= alpha;
        if (value < 0) value = 0;
        s->buffer[pos] = value;
        
        x++;
    }
}

/* Callback to render lines. */
static bool line_callback(const char *line, uint16_t count, void *state)
{
    state_t *s = (state_t*)state;
    
    if (s->options->justify)
    {
        mf_render_justified(s->font, s->options->anchor, s->y,
                            s->width - s->options->margin * 2,
                            line, count, pixel_callback, state);
    }
    else
    {
        mf_render_aligned(s->font, s->options->anchor, s->y,
                          s->options->alignment, line, count,
                          pixel_callback, state);
    }
    s->y += s->font->line_height;
    return true;
}

/* Callback to just count the lines.
 * Used to decide the image height */
bool count_lines(const char *line, uint16_t count, void *state)
{
    int *linecount = (int*)state;
    (*linecount)++;
    return true;
}

int main(int argc, const char **argv)
{
    int height;
    const struct mf_font_s *font;
    options_t options;
    state_t state = {};
    
    if (!parse_options(argc - 1, argv + 1, &options))
    {
        printf(
            "Usage: ./render_bmp [options] string\n"
            "Options:\n"
            "    -f font     Specify the font name to use.\n"
            "    -o out.bmp  Specify the output bmp file name.\n"
            "    -a l|c|r|j  Align left/center/right/justify.\n"
            "    -w width    Width of the image to render.\n"
            "    -m margin   Margin in the image.\n"
        );
        return 1;
    }
    
    font = mf_find_font(options.fontname, MF_INCLUDED_FONTS);
    
    if (!font)
    {
        printf("No such font: %s\n", options.fontname);
        return 2;
    }
    
    /* Count the number of lines that we need. */
    height = 0;
    mf_wordwrap(font, options.width - 2 * options.margin,
                options.text, count_lines, &height);
    height *= font->height;
    height += 4;
    
    /* Allocate and clear the image buffer */
    state.options = &options;
    state.width = options.width;
    state.height = height;
    state.buffer = malloc(options.width * height);
    state.y = 2;
    state.font = font;
    
    /* Initialize image to white */
    memset(state.buffer, 255, options.width * height);
    
    /* Render the text */
    mf_wordwrap(font, options.width - 2 * options.margin,
                options.text, line_callback, &state);
    
    /* Write out the bitmap */
    write_bmp(options.filename, state.buffer, state.width, state.height);
    
    printf("Wrote %s\n", options.filename);
    
    free(state.buffer);
    return 0;
}

