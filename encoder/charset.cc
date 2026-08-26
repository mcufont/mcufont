#include "charset.hh"
#include "gb2312_in_ucs2.h"
#include <cstdlib>
#include <stdexcept>

namespace mcufont {

namespace {

int parse_codepoint(const std::string &s)
{
    if (s.empty())
        throw std::invalid_argument("empty character specification");

    const char *begin = s.c_str();
    char *end = nullptr;
    long value = std::strtol(begin, &end, 0);

    if (end != begin + s.size())
        throw std::invalid_argument("invalid character specification '" + s + "'");

    if (value < 0 || value > 0xFFFF)
        throw std::invalid_argument("character out of range '" + s + "'");

    return static_cast<int>(value);
}

}

std::set<int> parse_charset(const std::vector<std::string> &args, size_t first)
{
    std::set<int> result;

    for (size_t i = first; i < args.size(); i++)
    {
        const std::string &s = args.at(i);

        if (s == "gb2312")
        {
            const size_t count = sizeof(gb2312_in_ucs2_codetable) /
                                 sizeof(gb2312_in_ucs2_codetable[0]);
            result.insert(&gb2312_in_ucs2_codetable[0],
                          &gb2312_in_ucs2_codetable[count]);
            continue;
        }

        // Look past index 0 so that a leading minus is a parse error rather
        // than an empty range start.
        size_t pos = s.find('-', 1);

        if (pos == std::string::npos)
        {
            result.insert(parse_codepoint(s));
        }
        else
        {
            int start = parse_codepoint(s.substr(0, pos));
            int end = parse_codepoint(s.substr(pos + 1));

            if (end < start)
                throw std::invalid_argument("reversed range '" + s + "'");

            for (int c = start; c <= end; c++)
                result.insert(c);
        }
    }

    return result;
}

}
