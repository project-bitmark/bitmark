#ifndef CLIENTVERSION_H
#define CLIENTVERSION_H

#if defined(HAVE_CONFIG_H)
#include "bitmark-config.h"
#else
//
// client versioning and copyright year
//

// These need to be macros, as version.cpp's and bitmark-qt.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR       0
#define CLIENT_VERSION_MINOR       9
#define CLIENT_VERSION_REVISION    7
#define CLIENT_VERSION_BUILD       3

// Set to true for release, false for prerelease (rc: release candidate)  or test build
#define CLIENT_VERSION_IS_RELEASE  false

// Copyright year (2009-this)

#define COPYRIGHT_YEAR 2021

#endif //HAVE_CONFIG_H

// Converts the parameter X to a string after macro replacement on X has been performed.
// Don't merge these into one macro!
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#endif // CLIENTVERSION_H
