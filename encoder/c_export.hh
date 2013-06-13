// Write out the encoded data in C source code files.

#pragma once

#include "datafile.hh"
#include "encode.hh"
#include <iostream>

void write_header(std::ostream &out, std::string name, const DataFile &datafile);

void write_source(std::ostream &out, std::string name, const DataFile &datafile);
