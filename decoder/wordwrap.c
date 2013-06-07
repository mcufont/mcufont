#include "wordwrap.h"
#include <stdbool.h>

/* Represents a single word and the whitespace after it. */
struct wordlen_s
{
    int16_t word; /* Length of the word in pixels. */
    int16_t space; /* Length of the whitespace in pixels. */
    uint16_t chars; /* Number of characters in word + space, combined. */
};

/* Take the next word from the string and compute its width.
 * Returns true if the word ends in a linebreak. */
static bool get_wordlen(const struct rlefont_s *font, const char **text,
                        struct wordlen_s *result)
{
    uint16_t c;
    const char *prev;
    
    result->word = 0;
    result->space = 0;
    result->chars = 0;
    
    c = utf8_getchar(text);
    while (c && !_isspace(c))
    {
        result->chars++;
        result->word += character_width(font, c);
        c = utf8_getchar(text);
    }
    
    prev = *text;
    while (c && _isspace(c))
    {
        result->chars++;
        
        if (c == ' ')
            result->space += character_width(font, c);
        else if (c == '\t')
            result->space += character_width(font, c) * TABSIZE;
        else if (c == '\n')
            break;
        
        prev = *text;
        c = utf8_getchar(text);
    }
    
    /* The last loop reads the first character of next word, put it back. */
    if (c)
        *text = prev;
    
    return (c == '\0' || c == '\n');
}

/* Represents the rendered length for a single line. */
struct linelen_s
{
    const char *start; /* Start of the text for line. */
    uint16_t chars; /* Total number of characters on the line. */
    int16_t width; /* Total length of all words + whitespace on the line in pixels. */
    bool linebreak; /* True if line ends in a linebreak */
    struct wordlen_s last_word; /* Last word on the line. */
    struct wordlen_s last_word_2; /* Second to last word on the line. */
};

/* Append word onto the line if it fits. If it would overflow, don't add and
 * return false. */
static bool append_word(const struct rlefont_s *font, int16_t width,
                        struct linelen_s *current, const char **text)
{
    const char *tmp = *text;
    struct wordlen_s wordlen;
    bool linebreak;
    
    linebreak = get_wordlen(font, &tmp, &wordlen);
    
    if (current->width + wordlen.word <= width)
    {
        *text = tmp;
        current->last_word_2 = current->last_word;
        current->last_word = wordlen;
        current->linebreak = linebreak;
        current->chars += wordlen.chars;
        current->width += wordlen.word + wordlen.space;
        return true;
    }
    else
    {
        return false;
    }
}

static int16_t abs16(int16_t x) { return (x > 0) ? x : -x; }
static int16_t sq16(int16_t x) { return x * x; }

/* Try to balance the lines by potentially moving one word from the previous
 * line to the the current one. */
static void tune_lines(struct linelen_s *current, struct linelen_s *previous,
                       int16_t max_width)
{
    int16_t curw1, prevw1;
    int16_t curw2, prevw2;
    int16_t delta1, delta2;
    
    /* If the lines are rendered as is */
    curw1 = current->width - current->last_word.space;
    prevw1 = previous->width - previous->last_word.space;
    delta1 = sq16(max_width - prevw1) + sq16(max_width - curw1);
    
    /* If the last word is moved */
    curw2 = current->width + previous->last_word.word;
    prevw2 = previous->width - previous->last_word.word
                             - previous->last_word.space
                             - previous->last_word_2.space;
    delta2 = sq16(max_width - prevw2) + sq16(max_width - curw2);
    
    if (delta1 > delta2 && curw2 <= max_width)
    {
        /* Do the change. */
        const char *tmp;
        uint16_t chars;
        
        previous->chars -= previous->last_word.chars;
        current->chars += previous->last_word.chars;
        previous->width -= previous->last_word.word + previous->last_word.space;
        current->width += previous->last_word.word + previous->last_word.space;
        previous->last_word = previous->last_word_2;
        
        tmp = previous->start;
        chars = previous->chars;
        while (chars--) utf8_getchar(&tmp);
        current->start = tmp;
    }
}

void wordwrap(const struct rlefont_s *font, int16_t width,
              const char *text, line_callback_t callback, void *state)
{
    struct linelen_s current = {};
    struct linelen_s previous = {};
    bool full;
    
    current.start = text;
    
    while (*text)
    {
        full = !append_word(font, width, &current, &text);
        
        if (full || current.linebreak)
        {
            if (previous.chars)
            {
                /* Tune the length and dispatch the previous line. */
                if (!previous.linebreak && !current.linebreak)
                    tune_lines(&current, &previous, width);
                
                callback(previous.start, previous.chars, state);
            }
            
            previous = current;
            current.start = text;
            current.chars = 0;
            current.width = 0;
            current.linebreak = false;
            current.last_word.word = 0;
            current.last_word.space = 0;
            current.last_word_2.word = 0;
            current.last_word_2.space = 0;
        }
    }
    
    /* Dispatch the last lines. */
    if (previous.chars)
        callback(previous.start, previous.chars, state);
    
    if (current.chars)
        callback(current.start, current.chars, state);
}
