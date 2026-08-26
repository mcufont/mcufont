# ESP-IDF component wrapper for the mcufont decoder.
#
# Only the decoder is built; the encoder is a host tool and is invoked from
# fonts/Makefile at development time, not from the firmware build.
#
# mf_font.c includes the generated fonts.h (MF_FONT_FILE_NAME), which lists
# the fonts compiled into the firmware. That file and the .c fonts it pulls in
# are project data rather than library data, so point MCUFONT_FONT_DIR at the
# directory holding them, from the top-level CMakeLists.txt before project():
#
#   set(MCUFONT_FONT_DIR "${CMAKE_SOURCE_DIR}/components/fonts")
#
# If it is unset, the fonts/ directory of this repository is used.

if(NOT MCUFONT_FONT_DIR)
    set(MCUFONT_FONT_DIR "${CMAKE_CURRENT_LIST_DIR}/fonts")
endif()

idf_component_register(
    SRCS    "decoder/mf_bwfont.c"
            "decoder/mf_encoding.c"
            "decoder/mf_font.c"
            "decoder/mf_justify.c"
            "decoder/mf_kerning.c"
            "decoder/mf_rlefont.c"
            "decoder/mf_scaledfont.c"
            "decoder/mf_wordwrap.c"
    INCLUDE_DIRS    "decoder"
                    "${MCUFONT_FONT_DIR}"
)
