// Global verbosity switch, set by the -v / --verbose command line option.
//
// Importing and optimizing a large font takes long enough that some progress
// output is useful, but it is noise for a scripted font build, so it is off
// by default.

#pragma once

namespace mcufont {

inline bool &verbose()
{
    static bool enabled = false;
    return enabled;
}

}
