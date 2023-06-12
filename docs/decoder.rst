==========================
MCUFont: Decoder reference
==========================

.. include :: menu.rst

.. contents ::



Overview of the decoder library
===============================
The decoder implementation is intended as an example and a basis for use in
custom applications. It can either be included verbatim, or modified to suit
a particular system. The decoder code is in public domain to allow inclusion
in any kind of project.

The decoder consists of several parts, of which most are optional and can be
left out if desired:

mf_encoding.c
  Character set support library, which can be configured to handle ASCII,
  UTF8, UTF16 or WCHAR strings on input. In ASCII configuration, only the
  mf_encoding.h file is needed.

mf_rlefont.c
  The rlefont decoder, which uncompresses the RLE compressed font files.
  You usually want to include this, because so far it is the only supported
  font format.

mf_kerning.c
  Optional automatic optical kerning algorithm. This adjusts the space between
  consecutive glyphs so that excessive space between combinations such as AW
  is removed.

mf_wordwrap.c
  Optional word wrapping algorithm. Breaks a long text into lines, while trying
  to balance the consecutive lines so that they are less ragged.

mf_justify.c
  Optional justification and alignment algorithms. Allows rendering a piece
  of text so that it is either left, center or right aligned or justified at
  both ends. This module can be used either for pre-wrapped text, or you can
  use the wordwrap module to wrap the text into lines.

mf_encoding: Character set library
==================================

mf_rlefont: Font decompression library
======================================

mf_kerning: Automatic kerning algorithm
=======================================

mf_wordwrap: Word wrapping algorithm
====================================

mf_justify: Justification and alignment algorithm
=================================================
