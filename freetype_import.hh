// Function for importing any font supported by libfreetype.

#pragma once
#include "datafile.hh"

std::unique_ptr<DataFile> LoadFreetype(std::istream &file);

