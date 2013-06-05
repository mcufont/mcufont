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

// Count the number of equal pixels at the beginning of the pixelstring.
static size_t prefix_length(const DataFile::pixels_t &pixels, size_t pos)
{
    uint8_t pixel = pixels.at(pos);
    size_t count = 1;
    while (pos + count < pixels.size() &&
            pixels.at(pos + count) == pixel)
    {
        count++;
    }
    return count;
}

// Perform the RLE encoding for a dictionary entry.
static encoded_font_t::rlestring_t encode_rle(const DataFile::pixels_t &pixels)
{
    encoded_font_t::rlestring_t result;
    
    size_t pos = 0;
    while (pos < pixels.size())
    {
        uint8_t pixel = pixels.at(pos);
        size_t count = prefix_length(pixels, pos);
        pos += count;
        
        if (pixel == 0)
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
        else if (pixel == 15)
        {
            // Encode ones.
            while (count)
            {
                size_t c = (count > 64) ? 64 : count;
                result.push_back(RLE_ONES | (c - 1));
                count -= c;
            }
        }
        else
        {
            // Encode shades.
            while (count)
            {
                size_t c = (count > 4) ? 4 : count;
                result.push_back(RLE_SHADE | ((c - 1) << 4) | pixel);
                count -= c;
            }
        }
    }
    
    return result;
}

// We use a tree structure to represent the dictionary entries.
// Using this tree, we can perform a greedy search for the matching entries.
// The resulting encoding may not be 100% optimal, but the algorithm is fast.
class DictTreeNode
{
public:
    constexpr DictTreeNode():
        m_index(-1),
        m_ref(false),
        m_child0(nullptr),
        m_child15(nullptr)
        {}
    
    void SetChild(uint8_t p, DictTreeNode *child)
    {
        if (p == 0)
            m_child0 = child;
        else if (p == 15)
            m_child15 = child;
        else if (p > 15)
            throw std::logic_error("invalid pixel alpha: " + std::to_string(p));
        else
        {
            if (!m_children)
            {
                m_children.reset(new DictTreeNode*[14]());
            }
            m_children[p - 1] = child;
        }
    }
    
    DictTreeNode* GetChild(uint8_t p) const
    { 
        if (p == 0)
            return m_child0;
        else if (p == 15)
            return m_child15;
        else if (p > 15)
            throw std::logic_error("invalid pixel alpha: " + std::to_string(p));
        else if (!m_children)
            return nullptr;
        else
            return m_children[p - 1];
    }
    
    int GetIndex() const { return m_index; }
    void SetIndex(int index) { m_index = index; }
    bool GetRef() const { return m_ref; }
    void SetRef(bool ref) { m_ref = ref; }
    
private:
    int m_index; // Index of dictionary entry or -1 if just a intermediate node.
    bool m_ref; // True for ref-encoded dictionary entries.
    
    // Most tree nodes will only ever contains children for 0 or 15.
    // Therefore the array for other nodes is allocated only on demand.
    DictTreeNode *m_child0;
    DictTreeNode *m_child15;
    std::unique_ptr<DictTreeNode*[]> m_children;
};

// Preallocated array for tree nodes
class TreeAllocator
{
public:
    TreeAllocator(size_t count)
    {
        m_storage.reset(new DictTreeNode[count]);
        m_next = m_storage.get();
        m_left = count;
    }
    
    DictTreeNode *allocate()
    {
        if (m_left == 0)
            throw std::logic_error("Ran out of preallocated entries");
        
        m_left--;
        return m_next++;
    }
    
private:
    std::unique_ptr<DictTreeNode[]> m_storage;
    DictTreeNode *m_next;
    size_t m_left;
};

// Construct a lookup tree from the dictionary entries.
static DictTreeNode* construct_tree(const std::vector<DataFile::dictentry_t> &dictionary, TreeAllocator &storage)
{
    DictTreeNode* root = storage.allocate();
    
    // Populate the hardcoded entries for 0 to 15 alpha.
    for (int j = 0; j < 16; j++)
    {
        DictTreeNode *node = storage.allocate();
        node->SetIndex(j);
        root->SetChild(j, node);
    }
    
    // Populate the rest of the entries
    size_t i = 0;
    for (DataFile::dictentry_t d : dictionary)
    {
        DictTreeNode* node = root;
        for (uint8_t p : d.replacement)
        {
            DictTreeNode* branch = node->GetChild(p);
            if (!branch)
            {
                branch = storage.allocate();
                node->SetChild(p, branch);
            }
            
            node = branch;
        }
        
        if (node->GetIndex() < 0)
        {
            node->SetIndex(i + DICT_START);
            node->SetRef(d.ref_encode);
        }
        i++;
    }
    
    return root;
}

// Walk the tree as far as possible following the given pixel string iterator.
// Returns number of pixels encoded, and index is set to the dictionary reference.
static size_t walk_tree(const DictTreeNode *tree,
                        DataFile::pixels_t::const_iterator pixels,
                        DataFile::pixels_t::const_iterator pixelsend,
                        int &index, bool is_glyph)
{
    size_t best_length = 0;
    size_t length = 0;
    index = -1;
    
    const DictTreeNode* node = tree;
    while (pixels != pixelsend)
    {
        uint8_t pixel = *pixels++;
        node = node->GetChild(pixel);
        
        if (!node)
            break;
        
        length++;
        
        if (is_glyph || !node->GetRef())
        {
            if (node->GetIndex() >= 0)
            {
                index = node->GetIndex();
                best_length = length;
            }
        }
    }
    
    if (index < 0)
        throw std::logic_error("walk_tree failed to find a valid encoding");
    
    return best_length;
}

// Perform the reference encoding for a glyph entry.
static encoded_font_t::refstring_t encode_ref(const DataFile::pixels_t &pixels,
                                              const DictTreeNode *tree,
                                              bool is_glyph)
{
    encoded_font_t::refstring_t result;
    
    // Strip any zeroes from end
    size_t end = pixels.size();
    
    if (is_glyph)
    {
        while (end > 0 && pixels.at(end - 1) == 0) end--;
    }
    
    size_t i = 0;
    while (i < end)
    {
        int index;
        i += walk_tree(tree, pixels.begin() + i, pixels.end(), index, is_glyph);
        result.push_back(index);
    }
    
    if (i < pixels.size())
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
    size_t count = DICT_START; // Preallocated entries
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
    DictTreeNode* tree = construct_tree(sorted_dict, allocator);
    
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
            std::unique_ptr<DataFile::pixels_t> decoded = 
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

std::unique_ptr<DataFile::pixels_t> decode_glyph(
    const encoded_font_t &encoded,
    const encoded_font_t::refstring_t &refstring,
    const DataFile::fontinfo_t &fontinfo)
{
    std::unique_ptr<DataFile::pixels_t> result(new DataFile::pixels_t);
    
    for (uint8_t ref : refstring)
    {
        if (ref <= 15)
        {
            result->push_back(ref);
        }
        else if (ref == REF_FILLZEROS)
        {
            result->resize(fontinfo.max_width * fontinfo.max_height, 0);
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
                        result->push_back(0);
                    }
                }
                else if ((rle & RLE_CODEMASK) == RLE_64ZEROS)
                {
                    for (int i = 0; i < ((rle & RLE_VALMASK) + 1) * 64; i++)
                    {
                        result->push_back(0);
                    }
                }
                else if ((rle & RLE_CODEMASK) == RLE_ONES)
                {
                    for (int i = 0; i < (rle & RLE_VALMASK) + 1; i++)
                    {
                        result->push_back(15);
                    }
                }
            }
        }
        else
        {
            size_t index = ref - DICT_START - encoded.rle_dictionary.size();
            std::unique_ptr<DataFile::pixels_t> part =
                decode_glyph(encoded, encoded.ref_dictionary.at(index),
                             fontinfo);
            result->insert(result->end(), part->begin(), part->end());
        }
    }
    
    return result;
}

std::unique_ptr<DataFile::pixels_t> decode_glyph(
    const encoded_font_t &encoded, size_t index,
    const DataFile::fontinfo_t &fontinfo)
{
    return decode_glyph(encoded, encoded.glyphs.at(index), fontinfo);
}
