// Given a dictionary and glyphs, encode the data for all the glyphs.

#pragma once

#include "datafile.hh"
#include <vector>
#include <memory>

struct encoded_font_t
{
    // RLE-encoded format for storing the dictionary entries.
    // Each item is a byte. Top bit means the value in the original bitstream,
    // and the bottom 7 bits store the repetition count.
    typedef std::vector<uint8_t> rlestring_t;
    
    // Reference encoded format for storing the glyphs.
    // Each item is a reference to the dictionary.
    // Values 0 and 1 are hardcoded to mean 0 and 1.
    // All other values mean dictionary entry at (i-2).
    typedef std::vector<uint8_t> refstring_t;
    
    std::vector<rlestring_t> dictionary;
    std::vector<refstring_t> glyphs;
};

// Encode all the glyphs.
std::unique_ptr<encoded_font_t> encode_font(const DataFile &datafile);

// Sum up the total size of the encoded glyphs + dictionary.
size_t get_encoded_size(const encoded_font_t &encoded);

inline size_t get_encoded_size(const DataFile &datafile)
{
    std::unique_ptr<encoded_font_t> e = encode_font(datafile);
    return get_encoded_size(*e);
}


// Decode a single glyph (for verification).
std::unique_ptr<DataFile::bitstring_t> decode_glyph(
    const encoded_font_t &encoded, size_t index,
    const DataFile::fontinfo_t &fontinfo);


#ifdef CXXTEST_RUNNING
#include <cxxtest/TestSuite.h>

class EncodeTests: public CxxTest::TestSuite
{
public:
    void testEncode()
    {
        std::istringstream s(testfile);
        std::unique_ptr<DataFile> f = DataFile::Load(s);
        std::unique_ptr<encoded_font_t> e = encode_font(*f);
        
        TS_ASSERT_EQUALS(e->glyphs.size(), 3);
        
        // Expected values for dictionary
        encoded_font_t::rlestring_t dict0 = {0x01, 0x81, 0x01, 0x81};
        encoded_font_t::rlestring_t dict1 = {0x04};
        encoded_font_t::rlestring_t dict2 = {0x84};
        
        TS_ASSERT(e->dictionary.at(0) == dict0);
        TS_ASSERT(e->dictionary.at(1) == dict1);
        TS_ASSERT(e->dictionary.at(2) == dict2);
        
        // Expected values for glyphs
        encoded_font_t::refstring_t glyph0 = {4, 4, 4, 4, 4, 4};
        encoded_font_t::refstring_t glyph1 = {4, 5, 5, 5, 5, 0, 0, 0, 1};
        encoded_font_t::refstring_t glyph2 = {5, 6, 0, 0, 0, 1, 1, 1, 5, 6, 2};
        
        TS_ASSERT(e->glyphs.at(0) == glyph0);
        TS_ASSERT(e->glyphs.at(1) == glyph1);
        TS_ASSERT(e->glyphs.at(2) == glyph2);
    }
    
    void testDecode()
    {
        std::istringstream s(testfile);
        std::unique_ptr<DataFile> f = DataFile::Load(s);
        std::unique_ptr<encoded_font_t> e = encode_font(*f);
        
        for (size_t i = 0; i < 3; i++)
        {
            std::unique_ptr<DataFile::bitstring_t> dec;
            dec = decode_glyph(*e, i, f->GetFontInfo());
            
            TS_ASSERT(*dec == f->GetGlyphEntry(i).data);
        }
    }
    
private:
    const char *testfile =
        "FontName Sans Serif\n"
        "MaxWidth 4\n"
        "MaxHeight 6\n"
        "BaselineX 1\n"
        "BaselineY 1\n"
        "DictEntry 1 0101\n"
        "DictEntry 1 0000\n"
        "DictEntry 1 1111\n"
        "Glyph 1 4 010101010101010101010101\n"
        "Glyph 2 4 010100000000000000000001\n"
        "Glyph 3 4 000011110001110000111100\n";
};
#endif