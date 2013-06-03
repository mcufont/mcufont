#include "c_export.hh"
#include <vector>
#include <iomanip>
#include <map>
#include <algorithm>
#include <string>
#include <cctype>

// Convert a file name to a valid C identifier
static std::string to_identifier(std::string name)
{
    // If the name contains path separators (/ or \), take only the last part.
    size_t pos = name.find_last_of("/\\");
    if (pos != std::string::npos)
        name = name.substr(pos + 1);
    
    // If the name contains a file extension, strip it.
    pos = name.find_first_of(".");
    if (pos != std::string::npos)
        name = name.substr(0, pos);
    
    // Replace any special characters with _.
    for (pos = 0; pos < name.size(); pos++)
    {
        if (!isalnum(name.at(pos)))
            name.at(pos) = '_';
    }
    
    return name;
}

// Write a vector of integers as line-wrapped hex/integer data for initializing const array.
static void wordwrap(std::ostream &out, const std::vector<unsigned> &data,
                     const std::string &prefix, size_t width = 2)
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

// Write a vector of integers as a C constant array of given datatype.
static void write_table(std::ostream &out, const std::vector<unsigned> &data,
                        const std::string &datatype, const std::string &tablename,
                        size_t width = 2)
{
    out << "static const " << datatype << " " << tablename;
    out << "[" << data.size() << "] = {" << std::endl;
    wordwrap(out, data, "    ", width);
    out << std::endl << "};" << std::endl;
    out << std::endl;
}

// Structure to represent one consecutive range of characters.
struct char_range_t
{
    uint16_t first_char;
    uint16_t char_count;
    std::vector<size_t> glyph_indices;
    
    char_range_t(): first_char(0), char_count(0) {}
};

// Find out all the characters present in the font and decide how to best
// to divide them into ranges.
std::vector<char_range_t> compute_char_ranges(const DataFile &datafile,
                                              const encoded_font_t &encoded)
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
        
        uint16_t last_char = chars.at(i - 1);
        
        // Then store the indices of glyphs for each character
        size_t data_length = 0;
        for (size_t j = range.first_char; j <= last_char; j++)
        {
            size_t glyph_index;
            if (char_to_glyph.count(j) == 0)
                glyph_index = datafile.GetFontInfo().default_glyph;
            else
                glyph_index = char_to_glyph[j];
            
            // We can encode at most 64 kB in a single character range.
            data_length += encoded.glyphs[glyph_index].size() + 1;
            if (data_length > 65535)
            {
                last_char = j - 1;
                break;
            }
            
            range.glyph_indices.push_back(glyph_index);
        }
        
        range.char_count = last_char - range.first_char + 1;
        result.push_back(range);
    }
    
    return result;
}

void write_header(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = to_identifier(name);
    
    out << std::endl;
    out << "/* Automatically generated font definition for font '" << name << "'. */" << std::endl;
    out << "#ifndef _" << name << "_H_" << std::endl;
    out << "#define _" << name << "_H_" << std::endl;
    out << std::endl;
    out << "#include \"rlefont.h\"" << std::endl;
    out << std::endl;
    out << "/* The font definition */" << std::endl;
    out << "extern const struct rlefont_s rlefont_" << name << ";" << std::endl;
    out << std::endl;
    out << "/* List entry for searching fonts by name. */" << std::endl;
    out << "static const struct rlefont_list_s rlefont_" << name << "_listentry = {" << std::endl;
    out << "#   ifndef INCLUDED_FONTS" << std::endl;
    out << "    0," << std::endl;
    out << "#   else" << std::endl;
    out << "    INCLUDED_FONTS," << std::endl;
    out << "#   undef INCLUDED_FONTS" << std::endl;
    out << "#   endif" << std::endl;
    out << "    &rlefont_" << name << std::endl;
    out << "};" << std::endl;
    out << "#define INCLUDED_FONTS (&rlefont_" << name << "_listentry)" << std::endl;
    
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
    
    write_table(out, data, "uint8_t", "dictionary_data");
    write_table(out, offsets, "uint16_t", "dictionary_offsets", 4);
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
    
    write_table(out, data, "uint8_t", "glyph_data_" + std::to_string(range_index));
    write_table(out, offsets, "uint16_t", "glyph_offsets_" + std::to_string(range_index), 4);
}

void write_source(std::ostream &out, std::string name, const DataFile &datafile)
{
    name = to_identifier(name);
    std::unique_ptr<encoded_font_t> encoded = encode_font(datafile, true);
    
    out << "/* Automatically generated font definition. */" << std::endl;
    out << "#include \"" << name << ".h\"" << std::endl;
    out << std::endl;
    
    // Write out the dictionary entries
    encode_dictionary(out, datafile, *encoded);
    
    // Write out glyph data for character ranges
    std::vector<char_range_t> ranges = compute_char_ranges(datafile, *encoded);
    for (size_t i = 0; i < ranges.size(); i++)
    {
        encode_character_range(out, datafile, *encoded, ranges.at(i), i);
    }
    
    // Write out a table describing the character ranges
    out << "static const struct char_range_s char_ranges[] = {" << std::endl;
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
    out << "const struct rlefont_s rlefont_" << name << " = {" << std::endl;
    out << "    " << "\"" << datafile.GetFontInfo().name << "\"," << std::endl;
    out << "    " << "\"" << name << "\"," << std::endl;
    out << "    " << "dictionary_data," << std::endl;
    out << "    " << "dictionary_offsets," << std::endl;
    out << "    " << encoded->rle_dictionary.size() << ", /* rle dict count */" << std::endl;
    out << "    " << encoded->ref_dictionary.size() + encoded->rle_dictionary.size() << ", /* total dict count */" << std::endl;
    out << "    " << "&glyph_data_0[0], /* default glyph */" << std::endl; // FIXME: Should use the real default glyph from BDF
    out << "    " << ranges.size() << ", /* char range count */" << std::endl;
    out << "    " << "char_ranges," << std::endl;
    out << "    " << datafile.GetFontInfo().max_width << ", /* width */" << std::endl;
    out << "    " << datafile.GetFontInfo().max_height << ", /* height */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_x << ", /* baseline x */" << std::endl;
    out << "    " << datafile.GetFontInfo().baseline_y << ", /* baseline y */" << std::endl;
    out << "};" << std::endl;
}


