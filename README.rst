=================
MCUFont: Overview
=================

.. include :: menu.rst

MCUFont is a font compression, decompression and rendering library for use with
microcontroller systems. Its main purpose is to allow high-quality anti-aliased
text rendering, while having small enough footprint to fit in the typical flash
memories.

Historically, there are many simple font rendering routines available. They are
usually ad-hoc implementations inside various graphics libraries, and
consequently they "take the easy way out". Usually this means monochrome only,
no kerning, sometimes only monospace and very basic algorithms for word wrap.
The goal of this library is to become a standard solution for this problem, so
that also microcontroller based systems can enjoy high quality text. On the
other hand, the purpose is not to compete with libfreetype and similar vector
font rendering libraries, because they already exist.


Overall structure
=================
The library consists of the encoder program, written in C++, and the decoder
library, written in ansi C. The encoder runs on your PC and is used to import
and compress font files. The decoder runs on the target microcontroller and
decompresses and renders the characters.


Getting started
===============
Once you download the library, run *make* in the root folder. This will build
the encoder, encode a few example fonts and build a *render_bmp* example
program for testing the decoder and renderer.

The example fonts will be built in the *fonts* subfolder, and consist of two
files each. For example, *DejaVuSans12.c* and *DejaVuSans12.h* are the
DejaVu Sans font rendered at 12 pixels height. To render text using this
font, you could do this::

    #include "fonts.h"
    #include <mcufont.h>

    static void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void *state)
        {
        while (count--)
            {
                /* your code goes here, ex: drawPixel(x, y, alpha, color::black); */
                x++;
            }
        }

    static uint8_t char_callback(int16_t x0, int16_t y0, mf_char character, void *state)
        {
            return mf_render_character(&mf_rlefont_DejaVuSans12.font, x0, y0, character, &pixel_callback, state);
        }

    void main()
        {
            mf_render_aligned(
                &mf_rlefont_DejaVuSans12.font,
                0, 0,
                MF_ALIGN_LEFT,
                "Hello, World!", 13,
                &char_callback, NULL);
        }


What happens here is that *mf_render_aligned* takes each character of the
string "Hello, world!" in turn, and calls *mf_render_character* for them.
The character rendering function will then decompress the glyph data and call
*pixel_callback* for each consecutive run of pixels. The callback function
provided by you will then finally draw the pixels to the screen.


Features and limitations
========================
*Features*

- Pure C runtime
- Support for importing .ttf and .bdf fonts
- Small code size of the decoder library (1-5 kB depending on enabled features)
- Abstract callback interface for writing to any kind of display
- 16-level antialiased fonts
- Uses libfreetype's high-quality hinting when importing .ttf fonts
- Advanced kerning, justification and word wrapping algorithms
- Fast decoding and high compression ratio

*Limitations*

- No support for runtime scaling of fonts
- Slow encoding
- No subpixel antialiasing (could be added in the future)


System requirements
===================
The decoder side has minimal system requirements. The following header files
are only required for the declaration of the standard types, and can be easily
replaced if not available on the target platform:

- *stdbool.h* for declaration of *bool* datatype
- *stdint.h* for declaration of *uint32_t* etc. datatypes
- *stddef.h* for *wchar_t* if enabled (optional)

The encoder also should compile on many kinds of platforms. It needs
libfreetype, cxxtest and a C++ compiler to build.


Debugging and testing
=====================
The encoder includes basic unit tests which are run before building. The
decoder side is tested using the *render_bmp* example applications. All tests
are run automatically by executing *make* in the top directory.
