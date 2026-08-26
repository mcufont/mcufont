// Parsing of character set specifications, shared by the commands that take
// one (import_ttf, filter).

#pragma once
#include <set>
#include <string>
#include <vector>

namespace mcufont {

// Parses args[first..] into a set of codepoints. Each argument is either a
// single codepoint ("65", "0x41"), an inclusive range ("0x30-0x39"), or the
// name of a predefined set ("gb2312").
//
// Parsing is strict: a token that is not fully consumed is an error rather
// than a silent truncation, so a typo like "0x10z" is reported instead of
// quietly becoming 0x10.
//
// Throws std::invalid_argument on a malformed argument.
std::set<int> parse_charset(const std::vector<std::string> &args, size_t first);

}
