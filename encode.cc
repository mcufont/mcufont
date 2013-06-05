#include "encode.hh"
#include <algorithm>
#include <stdexcept>

// Number of reserved codes before the dictionary entries.
#define DICT_START 24

// Special reference to mean "fill with zeros to the end of the glyph"
#define REF_FILLZEROS 16

// RLE codes
#define RLE_CODEMASK    0xC0
#define RLE_VALMASK     0x3F
#define RLE_ZEROS       0x00 // 0 to 63 zeros
#define RLE_64ZEROS     0x40 // (1 to 64) * 64 zeros
#define RLE_ONES        0x80 // 1 to 64 full alphas
#define RLE_SHADE       0xC0 // 1 to 4 partial alphas

// Count the number of equal bits at the beginning of the bitstring.
static size_t prefix_length(const DataFile::bitstring_t &bits, size_t pos)
{
    bool bit = bits.at(pos);
    size_t count = 1;
    while (pos + count < bits.size() &&
            bits.at(pos + count) == bit)
    {
        count++;
    }
    return count;
}

// Perform the RLE encoding for a dictionary entry.
static encoded_font_t::rlestring_t encode_rle(const DataFile::bitstring_t &bits)
{
    encoded_font_t::rlestring_t result;
    
    size_t pos = 0;
    while (pos < bits.size())
    {
        bool bit = bits.at(pos);
        size_t count = prefix_length(bits, pos);
        pos += count;
        
        if (bit == false)
        {
            // Up to 63 zeros can be encoded with RLE_ZEROS. If there are more,
            // encode using RLE_64ZEROS, and then whatever remains with RLE_ZEROS.
            while (count >= 64)
            {
                size_t c = (count > 4096) ? 64 : (count / 64);
                result.push_back(RLE_64ZEROS | (c - 1));
                count -= c * 64;
            }
            
            if (count)
            {
                result.push_back(RLE_ZEROS | count);
            }
        }
        else
        {
            // Encode ones.
            while (count)
            {
                size_t c = (count > 64) ? 64 : count;
                result.push_back(RLE_ONES | (c - 1));
                count -= c;
            }
        }
    }
    
    return result;
}

// We use a tree structure to represent the dictionary entries.
// Using this tree, we can perform a greedy search for the matching entries.
// The resulting encoding may not be 100% optimal, but the algorithm is fast.
struct dicttree_t
{
    int index; // Index of dictionary entry or -1 if just a intermediate node.
    dicttree_t *zero;
    dicttree_t *one;
    
    bool ref; // True for ref-encoded dictionary entries.
    
    constexpr dicttree_t(): index(-1), zero(nullptr), one(nullptr), ref(false) {}
    dicttree_t* &walk(bool b) { return b ? one : zero; }
    const dicttree_t* walk(bool b) const { return b ? one : zero; }
};

// Preallocated array for tree nodes
class TreeAllocator
{
public:
    TreeAllocator(size_t count)
    {
        m_storage.reset(new dicttree_t[count]);
        m_next = m_storage.get();
        m_left = count;
    }
    
    dicttree_t *allocate()
    {
        if (m_left == 0)
            throw std::logic_error("Ran out of preallocated entries");
        
        m_left--;
        return new (m_next++) dicttree_t;
    }
    
private:
    std::unique_ptr<dicttree_t[]> m_storage;
    dicttree_t *m_next;
    size_t m_left;
};

// Construct a lookup tree from the dictionary entries.
static dicttree_t* construct_tree(const std::vector<DataFile::dictentry_t> &dictionary, TreeAllocator &storage)
{
    dicttree_t* root = storage.allocate();
    
    // Populate the hardcoded entries for 0 and 255 alpha.
    root->zero = storage.allocate();
    root->zero->index = 0;
    root->one = storage.allocate();
    root->one->index = 15;
    
    // Populate the rest of the entries
    size_t i = 0;
    for (DataFile::dictentry_t d : dictionary)
    {
        dicttree_t* node = root;
        for (bool b : d.replacement)
        {
            dicttree_t* &branch = node->walk(b);
            if (!branch)
                branch = storage.allocate();
            
            node = branch;
        }
        
        if (node->index < 0)
        {
            node->index = i + DICT_START;
            node->ref = d.ref_encode;
        }
        i++;
    }
    
    return root;
}

// Walk the tree as far as possible following the given bitstring iterator.
// Returns number of bits encoded, and index is set to the dictionary reference.
static size_t walk_tree(const dicttree_t *tree,
                        DataFile::bitstring_t::const_iterator bits,
                        DataFile::bitstring_t::const_iterator bitsend,
                        int &index, bool is_glyph)
{
    size_t best_length = 0;
    size_t length = 0;
    index = -1;
    
    const dicttree_t* node = tree;
    while (bits != bitsend)
    {
        bool b = *bits++;
        node = node->walk(b);
        
        if (!node)
            break;
        
        length++;
        
        if (is_glyph || !node->ref)
        {
            if (node->index >= 0)
            {
                index = node->index;
                best_length = length;
            }
        }
    }
    
    if (index < 0)
        throw std::logic_error("walk_tree failed to find a valid encoding");
    
    return best_length;
}

// Perform the reference encoding for a glyph entry.
static encoded_font_t::refstring_t encode_ref(const DataFile::bitstring_t &bits,
                                              const dicttree_t *tree,
                                              bool is_glyph)
{
    encoded_font_t::refstring_t result;
    
    // Strip any zeroes from end
    size_t end = bits.size();
    
    if (is_glyph)
    {
        while (end > 0 && bits.at(end - 1) != true) end--;
    }
    
    size_t i = 0;
    while (i < end)
    {
        int index;
        i += walk_tree(tree, bits.begin() + i, bits.end(), index, is_glyph);
        result.push_back(index);
    }
    
    if (i < bits.size())
        result.push_back(REF_FILLZEROS);
    
    return result;
}

// Compare dictionary entries by their coding type.
// Sorts RLE-encoded entries first and any empty entries last.
static bool cmp_dict_coding(const DataFile::dictentry_t &a,
                            const DataFile::dictentry_t &b)
{
    if (a.replacement.size() == 0 && b.replacement.size() != 0)
        return false;
    else if (a.replacement.size() != 0 && b.replacement.size() == 0)
        return true;
    else if (a.ref_encode == false && b.ref_encode == true)
        return true;
    else
        return false;
}

size_t estimate_tree_node_count(const std::vector<DataFile::dictentry_t> &dict)
{
    size_t count = 3; // Preallocated entries
    for (const DataFile::dictentry_t &d: dict)
    {
        count += d.replacement.size();
    }
    return count;
}

std::unique_ptr<encoded_font_t> encode_font(const DataFile &datafile,
                                            bool verify)
{
    std::unique_ptr<encoded_font_t> result(new encoded_font_t);
    
    // Sort the dictionary so that RLE-coded entries come first.
    // This way the two are easy to distinguish based on index.
    std::vector<DataFile::dictentry_t> sorted_dict = datafile.GetDictionary();
    std::stable_sort(sorted_dict.begin(), sorted_dict.end(), cmp_dict_coding);
    
    // Build the binary tree for looking up references.
    size_t count = estimate_tree_node_count(sorted_dict);
    TreeAllocator allocator(count);
    dicttree_t* tree = construct_tree(sorted_dict, allocator);
    
    // Encode the dictionary entries, using either RLE or reference method.
    for (DataFile::dictentry_t d : sorted_dict)
    {
        if (d.replacement.size() == 0)
        {
            continue;
        }
        else if (d.ref_encode)
        {
            result->ref_dictionary.push_back(encode_ref(d.replacement, tree, false));
        }
        else
        {
            result->rle_dictionary.push_back(encode_rle(d.replacement));
        }
    }
    
    // Then reference-encode the glyphs
    for (DataFile::glyphentry_t g : datafile.GetGlyphTable())
    {
        result->glyphs.push_back(encode_ref(g.data, tree, true));
    }
    
    // Optionally verify that the encoding was correct.
    if (verify)
    {
        for (size_t i = 0; i < datafile.GetGlyphCount(); i++)
        {
            std::unique_ptr<DataFile::bitstring_t> decoded = 
                decode_glyph(*result, i, datafile.GetFontInfo());
            if (*decoded != datafile.GetGlyphEntry(i).data)
                throw std::logic_error("verification of glyph " + std::to_string(i) + " failed");
        }
    }
    
    return result;
}

size_t get_encoded_size(const encoded_font_t &encoded)
{
    size_t total = 0;
    for (const encoded_font_t::rlestring_t &r : encoded.rle_dictionary)
    {
        total += r.size();
        
        if (r.size() != 0)
            total += 2; // Offset table entry
    }
    for (const encoded_font_t::refstring_t &r : encoded.ref_dictionary)
    {
        total += r.size();
        
        if (r.size() != 0)
            total += 2; // Offset table entry
    }
    for (const encoded_font_t::refstring_t &r : encoded.glyphs)
    {
        total += r.size();
        total += 2; // Offset table entry
        total += 1; // Width table entry
    }
    return total;
}

std::unique_ptr<DataFile::bitstring_t> decode_glyph(
    const encoded_font_t &encoded,
    const encoded_font_t::refstring_t &refstring,
    const DataFile::fontinfo_t &fontinfo)
{
    std::unique_ptr<DataFile::bitstring_t> result(new DataFile::bitstring_t);
    
    for (uint8_t ref : refstring)
    {
        if (ref == 0)
        {
            result->push_back(false);
        }
        else if (ref == 15)
        {
            result->push_back(true);
        }
        else if (ref == REF_FILLZEROS)
        {
            result->resize(fontinfo.max_width * fontinfo.max_height, false);
        }
        else if (ref < DICT_START)
        {
            throw std::logic_error("unknown code: " + std::to_string(ref));
        }
        else if (ref - DICT_START < (int)encoded.rle_dictionary.size())
        {
            for (uint8_t rle : encoded.rle_dictionary.at(ref - DICT_START))
            {
                if ((rle & RLE_CODEMASK) == RLE_ZEROS)
                {
                    for (int i = 0; i < (rle & RLE_VALMASK); i++)
                    {
                        result->push_back(false);
                    }
                }
                else if ((rle & RLE_CODEMASK) == RLE_64ZEROS)
                {
                    for (int i = 0; i < ((rle & RLE_VALMASK) + 1) * 64; i++)
                    {
                        result->push_back(false);
                    }
                }
                else if ((rle & RLE_CODEMASK) == RLE_ONES)
                {
                    for (int i = 0; i < (rle & RLE_VALMASK) + 1; i++)
                    {
                        result->push_back(true);
                    }
                }
            }
        }
        else
        {
            size_t index = ref - DICT_START - encoded.rle_dictionary.size();
            std::unique_ptr<DataFile::bitstring_t> part =
                decode_glyph(encoded, encoded.ref_dictionary.at(index),
                             fontinfo);
            result->insert(result->end(), part->begin(), part->end());
        }
    }
    
    return result;
}

std::unique_ptr<DataFile::bitstring_t> decode_glyph(
    const encoded_font_t &encoded, size_t index,
    const DataFile::fontinfo_t &fontinfo)
{
    return decode_glyph(encoded, encoded.glyphs.at(index), fontinfo);
}
