// Function for importing any font supported by libfreetype.

#pragma once
#include "datafile.hh"
#include <set>

namespace mcufont {

// Imports a font at the given pixel size. If charset is non-null, only those
// codepoints are rasterized; everything else in the font is skipped, which is
// far cheaper than importing the whole face and filtering afterwards.
std::unique_ptr<DataFile> LoadFreetype(std::istream &file, int size, bool bw,
                                      const std::set<int> *charset = nullptr);

}
