#include "export_rlefont.hh"
#include <vector>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <cctype>
#include "exporttools.hh"

#define RLEFONT_FORMAT_VERSION 2

namespace mcufont {
namespace rlefont {

void write_header(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = filename_to_identifier(name);
    
    out << std::endl;
    out << "/* Automatically generated font definition for font '" << name << "'. */" << std::endl;
    out << "#ifndef _" << name << "_H_" << std::endl;
    out << "#define _" << name << "_H_" << std::endl;
    out << std::endl;
    out << "#include \"mf_rlefont.h\"" << std::endl;
    out << std::endl;
    out << "/* The font definition */" << std::endl;
    out << "extern const struct mf_rlefont_s mf_rlefont_" << name << ";" << std::endl;
    out << std::endl;
    out << "/* List entry for searching fonts by name. */" << std::endl;
    out << "static const struct mf_font_list_s mf_rlefont_" << name << "_listentry = {" << std::endl;
    out << "#   ifndef MF_INCLUDED_FONTS" << std::endl;
    out << "    0," << std::endl;
    out << "#   else" << std::endl;
    out << "    MF_INCLUDED_FONTS," << std::endl;
    out << "#   undef MF_INCLUDED_FONTS" << std::endl;
    out << "#   endif" << std::endl;
    out << "    (struct mf_font_s*)&mf_rlefont_" << name << std::endl;
    out << "};" << std::endl;
    out << "#define MF_INCLUDED_FONTS (&mf_rlefont_" << name << "_listentry)" << std::endl;
    
    out << std::endl;
    out << "#endif" << std::endl;
}

// Encode the dictionary entries and the offsets to them.
// Generates tables dictionary_data and dictionary_offsets.
static void encode_dictionary(std::ostream &out, const DataFile &datafile,
                              const encoded_font_t &encoded)
{
    std::vector<unsigned> offsets;
    std::vector<unsigned> data;
    for (const encoded_font_t::rlestring_t &r : encoded.rle_dictionary)
    {
        offsets.push_back(data.size());
        data.insert(data.end(), r.begin(), r.end());
    }
    
    for (const encoded_font_t::refstring_t &r : encoded.ref_dictionary)
    {
        offsets.push_back(data.size());
        data.insert(data.end(), r.begin(), r.end());
    }
    offsets.push_back(data.size());
    
    write_const_table(out, data, "uint8_t", "dictionary_data");
    write_const_table(out, offsets, "uint16_t", "dictionary_offsets", 4);
}

// Encode the data tables for a single character range.
// Generates tables glyph_data_i and glyph_offsets_i.
static void encode_character_range(std::ostream &out, const DataFile &datafile,
                              const encoded_font_t& encoded,
                              const char_range_t& range,
                              unsigned range_index)
{
    std::vector<unsigned> offsets;
    std::vector<unsigned> data;
    std::map<size_t, unsigned> already_encoded;
    
    for (size_t glyph_index : range.glyph_indices)
    {
        if (already_encoded.count(glyph_index))
        {
            offsets.push_back(already_encoded[glyph_index]);
        }
        else
        {
            const encoded_font_t::refstring_t &r = encoded.glyphs[glyph_index];
            
            offsets.push_back(data.size());
            already_encoded[glyph_index] = data.size();
            
            data.push_back(datafile.GetGlyphEntry(glyph_index).width);
            data.insert(data.end(), r.begin(), r.end());
        }
    }
    
    write_const_table(out, data, "uint8_t", "glyph_data_" + std::to_string(range_index));
    write_const_table(out, offsets, "uint16_t", "glyph_offsets_" + std::to_string(range_index), 4);
}

// Select the character to use as a fallback.
static int select_fallback_char(const DataFile &datafile)
{
    std::set<int> chars;
    
    size_t i = 0;
    for (const DataFile::glyphentry_t &g: datafile.GetGlyphTable())
    {
        for (size_t c: g.chars)
        {
            chars.insert(c);
        }
        i++;
    }
    
    if (chars.count(0xFFFD))
        return 0xFFFD; // Unicode replacement character
    
    if (chars.count(0))
        return 0; // Used by many BDF fonts as replacement char
    
    if (chars.count('?'))
        return '?';
    
    return ' ';
}

void write_source(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = filename_to_identifier(name);
    std::unique_ptr<encoded_font_t> encoded = encode_font(datafile, true);
    
    out << "/* Automatically generated font definition. */" << std::endl;
    out << "#define MF_RLEFONT_INTERNALS 1" << std::endl;
    out << "#include \"" << name << ".h\"" << std::endl;
    out << std::endl;
    
    out << "#ifndef MF_RLEFONT_VERSION_" << RLEFONT_FORMAT_VERSION << "_SUPPORTED" << std::endl;
    out << "#error The font file is not compatible with this version of mcufont." << std::endl;
    out << "#endif" << std::endl;
    out << std::endl;
    
    // Write out the dictionary entries
    encode_dictionary(out, datafile, *encoded);
    
    // Split the characters into ranges
    auto get_glyph_size = [&encoded](size_t i)
    {
        return encoded->glyphs[i].size();
    };
    std::vector<char_range_t> ranges = compute_char_ranges(datafile,
        get_glyph_size, 65536, 16);

    // Write out glyph data for character ranges
    for (size_t i = 0; i < ranges.size(); i++)
    {
        encode_character_range(out, datafile, *encoded, ranges.at(i), i);
    }
    
    // Write out a table describing the character ranges
    out << "static const struct mf_rlefont_char_range_s char_ranges[] = {" << std::endl;
    for (size_t i = 0; i < ranges.size(); i++)
    {
        out << "    {" << ranges.at(i).first_char
            << ", " << ranges.at(i).char_count
            << ", glyph_offsets_" << i
            << ", glyph_data_" << i << "}," << std::endl; 
    }
    out << "};" << std::endl;
    out << std::endl;
    
    // Pull it all together in the rlefont_s structure.
    out << "const struct mf_rlefont_s mf_rlefont_" << name << " = {" << std::endl;
    out << "    {" << std::endl;
    out << "    " << "\"" << datafile.GetFontInfo().name << "\"," << std::endl;
    out << "    " << "\"" << name << "\"," << std::endl;
    out << "    " << datafile.GetFontInfo().max_width << ", /* width */" << std::endl;
    out << "    " << datafile.GetFontInfo().max_height << ", /* height */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_x << ", /* baseline x */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_y << ", /* baseline y */" << std::endl;
    out << "    " << datafile.GetFontInfo().line_height << ", /* line height */" << std::endl;
    out << "    " << datafile.GetFontInfo().flags << ", /* flags */" << std::endl;
    out << "    " << select_fallback_char(datafile) << ", /* fallback character */" << std::endl;
    out << "    " << "&mf_rlefont_character_width," << std::endl;
    out << "    " << "&mf_rlefont_render_character," << std::endl;
    out << "    }," << std::endl;
    
    out << "    " << RLEFONT_FORMAT_VERSION << ", /* version */" << std::endl;
    out << "    " << "dictionary_data," << std::endl;
    out << "    " << "dictionary_offsets," << std::endl;
    out << "    " << encoded->rle_dictionary.size() << ", /* rle dict count */" << std::endl;
    out << "    " << encoded->ref_dictionary.size() + encoded->rle_dictionary.size() << ", /* total dict count */" << std::endl;
    out << "    " << ranges.size() << ", /* char range count */" << std::endl;
    out << "    " << "char_ranges," << std::endl;
    out << "};" << std::endl;
}

}}

