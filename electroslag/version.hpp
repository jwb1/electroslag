//  Electroslag Interactive Graphics System
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#pragma once

#if !defined(RC_INVOKED)
namespace electroslag {
    enum version {
        version_unknown = 0,
        version_rev1 = 1,
        version_current = version_rev1
    };
}
#endif

// Unfortunately, The Windows resource compiler doesn't understand integer constants.
// So we need to replicate the latest version number in macro.
#define ELECTROSLAG_VERSION_MAJOR 1
#define ELECTROSLAG_VERSION_MINOR 0
#define ELECTROSLAG_VERSION_BUILD 0
#define ELECTROSLAG_VERSION_REVISION 0

#define ELECTROSLAG_VERSION_STRING_STRINGIZE(s) #s
#define ELECTROSLAG_VERSION_STRING_EXPANDED(s) ELECTROSLAG_VERSION_STRING_STRINGIZE(s)
#define ELECTROSLAG_VERSION_STRING \
    ELECTROSLAG_VERSION_STRING_EXPANDED(ELECTROSLAG_VERSION_MAJOR) "." \
    ELECTROSLAG_VERSION_STRING_EXPANDED(ELECTROSLAG_VERSION_MINOR) "." \
    ELECTROSLAG_VERSION_STRING_EXPANDED(ELECTROSLAG_VERSION_BUILD) "." \
    ELECTROSLAG_VERSION_STRING_EXPANDED(ELECTROSLAG_VERSION_REVISION)
