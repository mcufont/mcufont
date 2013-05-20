// This implements the actual optimization passes of the compressor.

#include "datafile.hh"

// Perform a single optimization step, consisting itself of multiple passes
// of each of the optimization algorithms.
void optimize(DataFile &datafile, size_t iterations = 200);
