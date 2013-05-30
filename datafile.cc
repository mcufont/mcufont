#include "datafile.hh"
#include <sstream>
#include <algorithm>
#include <cctype>

DataFile::DataFile(const std::vector<dictentry_t> &dictionary,
                   const std::vector<glyphentry_t> &glyphs,
                   const fontinfo_t &fontinfo):
    m_dictionary(dictionary), m_glyphtable(glyphs), m_fontinfo(fontinfo)
{
    dictentry_t dummy = {};
    while (m_dictionary.size() < dictionarysize)
        m_dictionary.push_back(dummy);
    
    UpdateLowScoreIndex();
}

void DataFile::Save(std::ostream &file) const
{
    file << "FontName " << m_fontinfo.name << std::endl;
    file << "MaxWidth " << m_fontinfo.max_width << std::endl;
    file << "MaxHeight " << m_fontinfo.max_height << std::endl;
    file << "BaselineX " << m_fontinfo.baseline_x << std::endl;
    file << "BaselineY " << m_fontinfo.baseline_y << std::endl;
    file << "RandomSeed " << m_seed << std::endl;
    
    for (const dictentry_t &d : m_dictionary)
    {
        if (d.replacement.size() != 0)
        {
            file << "DictEntry " << d.score << " ";
            file << d.ref_encode << " " << d.replacement << std::endl;
        }
    }
    
    for (const glyphentry_t &g : m_glyphtable)
    {
        file << "Glyph ";
        for (size_t i = 0; i < g.chars.size(); i++)
        {
            if (i != 0) file << ',';
            file << g.chars.at(i);
        }
        file << " " << g.width << " " << g.data << std::endl;
    }
}

std::unique_ptr<DataFile> DataFile::Load(std::istream &file)
{
    fontinfo_t fontinfo = {};
    std::vector<dictentry_t> dictionary;
    std::vector<glyphentry_t> glyphtable;
    uint32_t seed = 1234;
    
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream input(line);
        std::string tag;
        
        input >> tag;
        
        if (tag == "FontName")
        {
            while (std::isspace(input.peek())) input.get();
            std::getline(input, fontinfo.name);
        }
        else if (tag == "MaxWidth")
        {
            input >> fontinfo.max_width;
        }
        else if (tag == "MaxHeight")
        {
            input >> fontinfo.max_height;
        }
        else if (tag == "BaselineX")
        {
            input >> fontinfo.baseline_x;
        }
        else if (tag == "BaselineY")
        {
            input >> fontinfo.baseline_y;
        }
        else if (tag == "RandomSeed")
        {
            input >> seed;
        }
        else if (tag == "DictEntry")
        {
            dictentry_t d = {};
            input >> d.score >> d.ref_encode >> d.replacement;
            dictionary.push_back(d);
        }
        else if (tag == "Glyph")
        {
            glyphentry_t g = {};
            std::string chars;
            input >> chars >> g.width >> g.data;
            
            std::istringstream charstream(chars);
            int c;
            while (charstream >> c)
                g.chars.push_back(c);
            
            glyphtable.push_back(g);
        }
    }
    
    std::unique_ptr<DataFile> result(new DataFile(dictionary, glyphtable, fontinfo));
    result->SetSeed(seed);
    return result;
}

void DataFile::SetDictionaryEntry(size_t index, const dictentry_t &value)
{
    m_dictionary.at(index) = value;
    
    if (index == m_lowscoreindex ||
        m_dictionary.at(m_lowscoreindex).score > value.score)
    {
        UpdateLowScoreIndex();
    }
}

std::string DataFile::GlyphToText(size_t index) const
{
    std::ostringstream os;
    
    for (int y = 0; y < m_fontinfo.max_height; y++)
    {
        for (int x = 0; x < m_fontinfo.max_width; x++)
        {
            size_t pos = y * m_fontinfo.max_width + x;
            os << (m_glyphtable.at(index).data.at(pos) ? "X" : ".");
        }
        os << std::endl;
    }
    
    return os.str();
}

void DataFile::UpdateLowScoreIndex()
{
    auto comparison = [](const dictentry_t &a, const dictentry_t &b)
    {
        return a.score < b.score;
    };
    
    auto iter = std::min_element(m_dictionary.begin(),
                                 m_dictionary.end(),
                                 comparison);
    
    m_lowscoreindex = iter - m_dictionary.begin();
}

std::ostream& operator<<(std::ostream& os, const DataFile::bitstring_t& str)
{
    for (bool b: str)
    {
        os << (b ? '1' : '0');
    }
    return os;
}

std::istream& operator>>(std::istream& is, DataFile::bitstring_t& str)
{
    char c;
    str.clear();
    
    while (isspace(is.peek())) is.get();
    
    while (is.get(c))
    {
        if (c == '0')
            str.push_back(false);
        else if (c == '1')
            str.push_back(true);
        else
            break;
    }
    
    return is;
}
