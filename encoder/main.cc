#include "bdf_import.hh"
#include "c_export.hh"
#include "datafile.hh"
#include "encode.hh"
#include "freetype_import.hh"
#include "optimize.hh"
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

static std::string strip_extension(std::string filename)
{
    size_t pos = filename.find_last_of('.');
    
    if (pos == std::string::npos)
    {
        return filename;
    }
    else
    {
        return filename.substr(0, pos);
    }
}

static const char *usage_msg =
    "Usage:\n"
    "   import <ttffile> <size>         Import a .ttf font into a data file.\n"
    "   import_bdf <bdffile>            Import a .bdf font into a data file.\n"
    "   export <datfile> <basename>     Export to .c and .h source code.\n"
    "   filter <datfile> <range> ...    Remove everything except specified characters.\n"
    "   size <datfile>                  Check the encoded size of the data file.\n"
    "   optimize <datfile>              Perform an optimization pass on the data file.\n"
    "   show_encoded <datfile>          Show the encoded data for debugging.\n"
    "   show_glyph <datfile> <index>    Show the glyph at index.\n";

int main(int argc, char **argv)
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);
    
    if (args.size() == 3 && args.at(0) == "import")
    {
        std::string src = args.at(1);
        int size = std::stoi(args.at(2));
        std::string dest = strip_extension(args.at(1)) + std::to_string(size) + ".dat";
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::cout << "Importing " << src << " to " << dest << std::endl;
        
        std::unique_ptr<DataFile> f = LoadFreetype(infile, size);
        
        init_dictionary(*f);
        
        std::ofstream outfile(dest);
        f->Save(outfile);
        
        std::cout << "Done: " << f->GetGlyphCount() << " unique glyphs." << std::endl;
        return 0;
    }
    else if (args.size() == 2 && args.at(0) == "import_bdf")
    {
        std::string src = args.at(1);
        std::string dest = strip_extension(args.at(1)) + ".dat";
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::cout << "Importing " << src << " to " << dest << std::endl;
        
        std::unique_ptr<DataFile> f = LoadBDF(infile);
        
        init_dictionary(*f);
        
        std::ofstream outfile(dest);
        f->Save(outfile);
        
        std::cout << "Done: " << f->GetGlyphCount() << " unique glyphs." << std::endl;
        return 0;
    }
    else if (args.size() == 3 && args.at(0) == "export")
    {
        std::string src = args.at(1);
        std::string dst = args.at(2);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        
        {
            std::ofstream header(dst + ".h");
            write_header(header, dst, *f);
            std::cout << "Wrote " << dst << ".h" << std::endl;
        }
        
        {
            std::ofstream source(dst + ".c");
            write_source(source, dst, *f);
            std::cout << "Wrote " << dst << ".c" << std::endl;
        }
        
        return 0;
    }
    else if (args.size() >= 3 && args.at(0) == "filter")
    {
        std::set<int> allowed;
        
        // Parse arguments
        for (size_t i = 2; i < args.size(); i++)
        {
            std::string s = args.at(i);
            size_t pos = s.find('-');
            if (pos == std::string::npos)
            {
                // Single char
                allowed.insert(std::stoi(s, nullptr, 0));
            }
            else
            {
                // Range
                int start = std::stoi(s.substr(0, pos), nullptr, 0);
                int end = std::stoi(s.substr(pos + 1), nullptr, 0);
                
                for (int j = start; j <= end; j++)
                {
                    allowed.insert(j);
                }
            }
        }
        
        std::string src = args.at(1);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        
        std::cout << "Font originally had " << f->GetGlyphCount() << " glyphs." << std::endl;
        
        // Filter the glyphs
        std::vector<DataFile::glyphentry_t> newglyphs;
        for (size_t i = 0; i < f->GetGlyphCount(); i++)
        {
            DataFile::glyphentry_t g = f->GetGlyphEntry(i);
            
            for (size_t j = 0; j < g.chars.size(); j++)
            {
                if (!allowed.count(g.chars.at(j)))
                {
                    g.chars.erase(g.chars.begin() + j);
                    j--;
                }
            }
            
            if (g.chars.size())
            {
                newglyphs.push_back(g);
            }
        }
        
        f.reset(new DataFile(f->GetDictionary(), newglyphs, f->GetFontInfo()));
        std::cout << "After filtering, " << f->GetGlyphCount() << " glyphs remain." << std::endl;
        std::ofstream outfile(src);
        f->Save(outfile);
        return 0;
    }
    else if (args.size() == 2 && args.at(0) == "size")
    {
        std::string src = args.at(1);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        size_t size = get_encoded_size(*f);
        
        std::cout << "Current size of " << src << " is " << size << std::endl;
        return 0;
    }
    else if (args.size() >= 2 && args.at(0) == "optimize")
    {
        std::string src = args.at(1);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        size_t oldsize = get_encoded_size(*f);
        
        std::cout << "Original size is " << oldsize << " bytes" << std::endl;
        std::cout << "Press ctrl-C at any time to stop." << std::endl;
        std::cout << "Results are saved automatically after each iteration." << std::endl;
        
        int limit = 100;
        if (args.size() == 3)
        {
            limit = atoi(args.at(2).c_str());
        }
        
        if (limit > 0)
            std::cout << "Limit is " << limit << " iterations" << std::endl;
        
        int i = 0;
        time_t oldtime = time(NULL);
        while (!limit || i < limit)
        {
            optimize(*f);

            size_t newsize = get_encoded_size(*f);
            time_t newtime = time(NULL);
            
            int bytes_per_min = (oldsize - newsize) * 60 / (newtime - oldtime + 1);
            
            i++;
            std::cout << "iteration " << i << ", size " << newsize
                      << " bytes, speed " << bytes_per_min << " B/min"
                      << std::endl;
            
            {
                std::ofstream outfile(src);
                f->Save(outfile);
            }
        }
        
        return 0;
    }
    else if (args.size() == 2 && args.at(0) == "show_encoded")
    {
        std::string src = args.at(1);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        std::unique_ptr<encoded_font_t> e = encode_font(*f);
    
        int i = 0;
        for (encoded_font_t::rlestring_t d : e->rle_dictionary)
        {
            std::cout << "Dict RLE " << 24 + i++ << ": ";
            for (uint8_t v : d)
                std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)v << " ";
            std::cout << std::endl;
        }
        
        for (encoded_font_t::refstring_t d : e->ref_dictionary)
        {
            std::cout << "Dict Ref " << 24 + i++ << ": ";
            for (uint8_t v : d)
                std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)v << " ";
            std::cout << std::endl;
        }
        
        i = 0;
        for (encoded_font_t::refstring_t g : e->glyphs)
        {
            std::cout << "Glyph " << i++ << ": ";
            for (uint8_t v : g)
                std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)v << " ";
            std::cout << std::endl;
        }
    }
    else if (args.size() == 3 && args.at(0) == "show_glyph")
    {
        std::string src = args.at(1);
        std::ifstream infile(src);
        
        if (!infile.good())
        {
            std::cerr << "Could not open " << src << std::endl;
            return 1;
        }
        
        std::unique_ptr<DataFile> f = DataFile::Load(infile);
        
        size_t index = 0;
        if (args.at(2) == "largest")
        {
            std::unique_ptr<encoded_font_t> e = encode_font(*f);
            size_t maxlen = 0;
            size_t i = 0;
            for (encoded_font_t::refstring_t g : e->glyphs)
            {
                if (g.size() > maxlen)
                {
                    maxlen = g.size();
                    index = i;
                }
                i++;
            }
            
            std::cout << "Index " << index << ", length " << maxlen << std::endl;
        }
        else
        {
            index = strtol(args.at(2).c_str(), nullptr, 0);
        }
            
        if (index < 0 || index >= f->GetGlyphCount())
        {
            std::cerr << "No such glyph " << index << std::endl;
            return 2;
        }
        
        std::cout << f->GlyphToText(index);
        return 0;
    }
    else
    {
        std::cout << usage_msg << std::endl;
        return 1;
    }
}
