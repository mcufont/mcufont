#include "c_export.hh"
#include <vector>
#include <iomanip>
#include <map>
#include <algorithm>

// Write a vector of integers as line-wrapped hex/integer data for initializing const array.
static void wordwrap(std::ostream &out, const std::vector<unsigned> &data, const char *prefix, size_t width = 2)
{
    int values_per_column = (width <= 2) ? 16 : 8;
    
    std::ios::fmtflags flags(out.flags());
    out << prefix;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < data.size(); i++)
    {
        if (i % values_per_column == 0 && i != 0)
            out << std::endl << prefix;
        
        out << "0x" << std::setw(width) << (int)data.at(i) << ", ";
    }
    out.flags(flags);
}

struct char_range_t
{
    uint16_t first_char;
    uint16_t char_count;
    std::vector<size_t> glyph_indices;
    
    char_range_t(): first_char(0), char_count(0) {}
};

// Find out all the characters present in the font and decide how to best
// to divide them into ranges.
std::vector<char_range_t> compute_char_ranges(const DataFile &datafile)
{
    std::vector<char_range_t> result;
    std::map<size_t, size_t> char_to_glyph;
    std::vector<size_t> chars;
    
    size_t i = 0;
    for (const DataFile::glyphentry_t &g: datafile.GetGlyphTable())
    {
        for (size_t c: g.chars)
        {
            char_to_glyph[c] = i;
            chars.push_back(c);
        }
        i++;
    }
    
    std::sort(chars.begin(), chars.end());
    
    i = 0;
    while (i < chars.size())
    {
        char_range_t range;
        range.first_char = chars.at(i);
        
        // Find the point where there is a gap of more than 8 characters.
        i++;
        while (i < chars.size() && chars.at(i) - chars.at(i - 1) < 8)
            i++;
        
        range.char_count = chars.at(i - 1) + 1 - range.first_char;
        
        // Then store the indices of glyphs for each character
        for (size_t j = range.first_char;
             j < range.first_char + range.char_count; j++)
        {
            if (char_to_glyph.count(j) == 0)
            {
                // FIXME: This should use the default character.
                range.glyph_indices.push_back(0);
            }
            else
            {
                range.glyph_indices.push_back(char_to_glyph[j]);
            }
        }
        result.push_back(range);
    }
    
    return result;
}

void write_header(std::ostream &out, std::string name, const DataFile &datafile)
{
    out << "/* Automatically generated font definition. */" << std::endl;
    out << "#ifndef _" << name << "_H_" << std::endl;
    out << "#define _" << name << "_H_" << std::endl;
    out << std::endl;
    out << "#include \"rlefont.h\"" << std::endl;
    out << std::endl;
    out << "extern const struct rlefont_s rlefont_" << name << ";" << std::endl;
    out << std::endl;
    out << "#endif" << std::endl;
}

void write_source(std::ostream &out, std::string name, const DataFile &datafile)
{
    std::unique_ptr<encoded_font_t> encoded = encode_font(datafile, true);
    std::vector<unsigned> dictionary_offsets;
    std::vector<unsigned> glyph_offsets;
    std::vector<char_range_t> char_ranges;
    
    out << "/* Automatically generated font definition. */" << std::endl;
    out << "#include \"" << name << ".h\"" << std::endl;
    out << std::endl;
    
    // 1. Encode the dictionary
    out << "static const uint8_t dictionary_data[] = {" << std::endl;
    std::vector<unsigned> data;
    for (const encoded_font_t::rlestring_t &r : encoded->rle_dictionary)
    {
        dictionary_offsets.push_back(data.size());
        data.insert(data.end(), r.begin(), r.end());
    }
    
    for (const encoded_font_t::refstring_t &r : encoded->ref_dictionary)
    {
        dictionary_offsets.push_back(data.size());
        data.insert(data.end(), r.begin(), r.end());
    }
    wordwrap(out, data, "    ");
    out << std::endl << "};" << std::endl;
    out << std::endl;
    out << "static const uint16_t dictionary_offsets[] = {" << std::endl;
    wordwrap(out, dictionary_offsets, "    ", 4);
    out << std::endl << "};" << std::endl;
    out << std::endl;
    
    // 2. Encode the glyphs
    out << "static const uint8_t glyph_data[] = {" << std::endl;
    data.clear();
    size_t i = 0;
    for (const encoded_font_t::refstring_t &r : encoded->glyphs)
    {
        glyph_offsets.push_back(data.size());
        
        // Encode the width of the glyph followed by the data.
        data.push_back(datafile.GetGlyphEntry(i).width);
        data.insert(data.end(), r.begin(), r.end());
        i++;
    }
    wordwrap(out, data, "    ");
    out << std::endl << "};" << std::endl;
    out << std::endl;
    
    // 3. Encode the character ranges
    std::vector<char_range_t> ranges = compute_char_ranges(datafile);
    i = 0;
    for (const char_range_t &range: ranges)
    {
        data.clear();
        out << "static const uint16_t glyph_offsets_" << i++ << "[] = {" << std::endl;
        for (size_t glyph_index : range.glyph_indices)
        {
            data.push_back(glyph_offsets.at(glyph_index));
        }
        wordwrap(out, data, "    ", 4);
        out << std::endl << "};" << std::endl;
        out << std::endl;
    }
    out << "static const struct char_range_s char_ranges[] = {" << std::endl;
    i = 0;
    for (const char_range_t &range: ranges)
    {
        out << "    {" << range.first_char
            << ", " << range.char_count
            << ", glyph_offsets_" << i++ << "}," << std::endl; 
    }
    out << "};" << std::endl;
    out << std::endl;
    
    // 4. Pull it all together in the rlefont_s structure.
    out << "const struct rlefont_s rlefont_" << name << " = {" << std::endl;
    out << "    " << "\"" << datafile.GetFontInfo().name << "\"," << std::endl;
    out << "    " << "dictionary_data," << std::endl;
    out << "    " << "dictionary_offsets," << std::endl;
    out << "    " << encoded->rle_dictionary.size() << ", /* rle dict count */" << std::endl;
    out << "    " << encoded->ref_dictionary.size() + encoded->rle_dictionary.size() << ", /* total dict count */" << std::endl;
    out << "    " << "glyph_data," << std::endl;
    out << "    " << glyph_offsets.at(0) << ", /* default glyph */" << std::endl; // FIXME: Should use the real default glyph from BDF
    out << "    " << ranges.size() << ", /* char range count */" << std::endl;
    out << "    " << "char_ranges," << std::endl;
    out << "    " << datafile.GetFontInfo().max_width << ", /* width */" << std::endl;
    out << "    " << datafile.GetFontInfo().max_height << ", /* height */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_x << ", /* baseline x */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_y << ", /* baseline y */" << std::endl;
    out << "};" << std::endl;
}


