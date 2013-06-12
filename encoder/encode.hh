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
    
    std::vector<rlestring_t> rle_dictionary;
    std::vector<refstring_t> ref_dictionary;
    std::vector<refstring_t> glyphs;
};

// Encode all the glyphs.
std::unique_ptr<encoded_font_t> encode_font(const DataFile &datafile,
                                            bool verify = false);

// Sum up the total size of the encoded glyphs + dictionary.
size_t get_encoded_size(const encoded_font_t &encoded);

inline size_t get_encoded_size(const DataFile &datafile)
{
    std::unique_ptr<encoded_font_t> e = encode_font(datafile);
    return get_encoded_size(*e);
}


// Decode a single glyph (for verification).
std::unique_ptr<DataFile::pixels_t> decode_glyph(
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
        encoded_font_t::rlestring_t dict0 = {0x01, 0x80, 0x01, 0x80};
        encoded_font_t::rlestring_t dict1 = {0x04};
        encoded_font_t::rlestring_t dict2 = {0x83};
        encoded_font_t::refstring_t dict3 = {24, 24};
        
        TS_ASSERT(e->rle_dictionary.at(0) == dict0);
        TS_ASSERT(e->rle_dictionary.at(1) == dict1);
        TS_ASSERT(e->rle_dictionary.at(2) == dict2);
        TS_ASSERT(e->ref_dictionary.at(0) == dict3);
        
        // Expected values for glyphs
        encoded_font_t::refstring_t glyph0 = {27, 27, 27};
        encoded_font_t::refstring_t glyph1 = {24, 25, 25, 25, 25, 0, 0, 0, 15};
        encoded_font_t::refstring_t glyph2 = {25, 26, 0, 0, 0, 15, 15, 15, 25, 26, 16};
        
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
            std::unique_ptr<DataFile::pixels_t> dec;
            dec = decode_glyph(*e, i, f->GetFontInfo());
            
            TS_ASSERT(*dec == f->GetGlyphEntry(i).data);
        }
    }
    
private:
    const char *testfile =
        "Version 1\n"
        "FontName Sans Serif\n"
        "MaxWidth 4\n"
        "MaxHeight 6\n"
        "BaselineX 1\n"
        "BaselineY 1\n"
        "DictEntry 1 0 0F0F\n"
        "DictEntry 1 0 0000\n"
        "DictEntry 1 0 FFFF\n"
        "DictEntry 1 1 0F0F0F0F\n"
        "Glyph 1 4 0F0F0F0F0F0F0F0F0F0F0F0F\n"
        "Glyph 2 4 0F0F0000000000000000000F\n"
        "Glyph 3 4 0000FFFF000FFF0000FFFF00\n";
};
#endif