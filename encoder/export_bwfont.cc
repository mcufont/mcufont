#include "export_bwfont.hh"
#include <vector>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <cctype>
#include "exporttools.hh"
#include "importtools.hh"

#define BWFONT_FORMAT_VERSION 1

namespace mcufont {
namespace bwfont {

void write_header(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = filename_to_identifier(name);
    
    out << "/* Automatically generated font definition for font '" << name << "'. */" << std::endl;
    out << "#ifndef _" << name << "_H_" << std::endl;
    out << "#define _" << name << "_H_" << std::endl;
    out << std::endl;
    out << "#include \"mf_bwfont.h\"" << std::endl;
    out << std::endl;
    out << "/* The font definition */" << std::endl;
    out << "extern const struct mf_bwfont_s mf_bwfont_" << name << ";" << std::endl;
    out << std::endl;
    out << "/* List entry for searching fonts by name. */" << std::endl;
    out << "static const struct mf_font_list_s mf_bwfont_" << name << "_listentry = {" << std::endl;
    out << "    MF_INCLUDED_FONTS," << std::endl;
    out << "    (struct mf_font_s*)&mf_bwfont_" << name << std::endl;
    out << "};" << std::endl;
    out << "#undef MF_INCLUDED_FONTS" << std::endl;
    out << "#define MF_INCLUDED_FONTS (&mf_bwfont_" << name << "_listentry)" << std::endl;
    
    out << std::endl;
    out << "#endif" << std::endl;
    
}

static void encode_glyph(const DataFile::glyphentry_t &glyph,
                         const DataFile::fontinfo_t &fontinfo,
                         std::vector<unsigned> &dest)
{
    for (int x = 0; x < glyph.width; x++)
    {
        for (int y = 0; y < fontinfo.max_height; y+= 8)
        {
            size_t remain = std::min(8, fontinfo.max_height - y);
            uint8_t byte = 0;
            for (size_t i = 0; i < remain; i++)
            {
                size_t index = (y + i) * fontinfo.max_width + x;
                if (glyph.data.at(index) >= 8)
                {
                    byte |= (1 << i);
                }
            }
            dest.push_back(byte);
        }
    }
}

struct cropinfo_t
{
    size_t offset_x;
    size_t offset_y;
    size_t height_bytes;
    size_t height_pixels;
};

static void encode_character_range(std::ostream &out, const DataFile &datafile,
                                   const char_range_t &range,
                                   unsigned range_index,
                                   cropinfo_t &cropinfo)
{
    std::vector<DataFile::glyphentry_t> glyphs;
    
    // Copy all the glyphs in this range for the purpose of cropping them.
    for (int glyph_index: range.glyph_indices)
    {
        if (glyph_index < 0)
        {
            // Missing glyph
            DataFile::glyphentry_t dummy = {};
            glyphs.push_back(dummy);
        }
        else
        {
            glyphs.push_back(datafile.GetGlyphEntry(glyph_index));
        }
    }
    
    // Crop the glyphs in this range. Getting rid of a few rows at top
    // or left can save a bunch of bytes with minimal cost.
    DataFile::fontinfo_t old_fi = datafile.GetFontInfo();
    DataFile::fontinfo_t new_fi = old_fi;
    crop_glyphs(glyphs, new_fi);
    
    // Fill in the crop information
    cropinfo.offset_x = old_fi.baseline_x - new_fi.baseline_x;
    cropinfo.offset_y = old_fi.baseline_y - new_fi.baseline_y;
    cropinfo.height_pixels = new_fi.max_height;
    cropinfo.height_bytes = (cropinfo.height_pixels + 7) / 8;
    
    // Then format and write out the glyph data
    std::vector<unsigned> offsets;
    std::vector<unsigned> data;
    
    for (const DataFile::glyphentry_t &g : glyphs)
    {
        offsets.push_back(data.size());
        encode_glyph(g, new_fi, data);
    }    
    offsets.push_back(data.size());
    
    write_const_table(out, data, "uint8_t", "glyph_data_" + std::to_string(range_index));
    write_const_table(out, offsets, "uint16_t", "glyph_offsets_" + std::to_string(range_index), 4);
}
    
void write_source(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = filename_to_identifier(name);
    
    out << "/* Automatically generated font definition. */" << std::endl;
    out << "#define MF_BWFONT_INTERNALS 1" << std::endl;
    out << "#include \"" << name << ".h\"" << std::endl;
    out << std::endl;
    
    out << "#ifndef MF_BWFONT_VERSION_" << BWFONT_FORMAT_VERSION << "_SUPPORTED" << std::endl;
    out << "#error The font file is not compatible with this version of mcufont." << std::endl;
    out << "#endif" << std::endl;
    out << std::endl;
    
    // Split the characters into ranges
    DataFile::fontinfo_t f = datafile.GetFontInfo();
    size_t glyph_size = f.max_width * ((f.max_height + 7) / 8);
    auto get_glyph_size = [=](size_t i) { return glyph_size; };
    std::vector<char_range_t> ranges = compute_char_ranges(datafile,
        get_glyph_size, 65536, 16);

    // Write out glyph data for character ranges
    std::vector<cropinfo_t> crops;
    for (size_t i = 0; i < ranges.size(); i++)
    {
        cropinfo_t cropinfo;
        encode_character_range(out, datafile, ranges.at(i), i, cropinfo);
        crops.push_back(cropinfo);
    }
    
    // Write out a table describing the character ranges
    out << "static const struct mf_bwfont_char_range_s char_ranges[] = {" << std::endl;
    for (size_t i = 0; i < ranges.size(); i++)
    {
        out << "    {" << std::endl;
        out << "        " << ranges.at(i).first_char << ", /* first char */" << std::endl;
        out << "        " << ranges.at(i).char_count << ", /* char count */" << std::endl;
        out << "        " << crops[i].offset_x << ", /* offset x */" << std::endl;
        out << "        " << crops[i].offset_y << ", /* offset y */" << std::endl;
        out << "        " << crops[i].height_bytes << ", /* height in bytes */" << std::endl;
        out << "        " << crops[i].height_pixels << ", /* height in pixels */" << std::endl;
        out << "        " << "glyph_offsets_" << i << "," << std::endl;
        out << "        " << "glyph_data_" << i << "," << std::endl;
        out << "    }," << std::endl;
    }
    out << "};" << std::endl;
    out << std::endl;
    
    // Fonts in this format are always black & white
    int flags = datafile.GetFontInfo().flags | DataFile::FLAG_BW;
    
    // Pull it all together in the rlefont_s structure.
    out << "const struct mf_bwfont_s mf_bwfont_" << name << " = {" << std::endl;
    out << "    {" << std::endl;
    out << "    " << "\"" << datafile.GetFontInfo().name << "\"," << std::endl;
    out << "    " << "\"" << name << "\"," << std::endl;
    out << "    " << datafile.GetFontInfo().max_width << ", /* width */" << std::endl;
    out << "    " << datafile.GetFontInfo().max_height << ", /* height */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_x << ", /* baseline x */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_y << ", /* baseline y */" << std::endl;
    out << "    " << datafile.GetFontInfo().line_height << ", /* line height */" << std::endl;
    out << "    " << flags << ", /* flags */" << std::endl;
    out << "    " << select_fallback_char(datafile) << ", /* fallback character */" << std::endl;
    out << "    " << "&mf_bwfont_character_width," << std::endl;
    out << "    " << "&mf_bwfont_render_character," << std::endl;
    out << "    }," << std::endl;
    
    out << "    " << BWFONT_FORMAT_VERSION << ", /* version */" << std::endl;
    out << "    " << ranges.size() << ", /* char range count */" << std::endl;
    out << "    " << "char_ranges," << std::endl;
    out << "};" << std::endl;
}
    
    
}}
