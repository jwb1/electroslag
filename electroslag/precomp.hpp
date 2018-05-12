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

// Decode ELECTROSLAG_BUILD flag; replicated in resource.rc
#if ELECTROSLAG_BUILD==1
#define ELECTROSLAG_BUILD_DEBUG
#elif ELECTROSLAG_BUILD==2
#define ELECTROSLAG_BUILD_RELEASE
#elif ELECTROSLAG_BUILD==3
#define ELECTROSLAG_BUILD_SHIP
#else
#error ELECTROSLAG_BUILD is undefined!
#endif

// Set Microsoft-specific C Run Time Library standard header file options.
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

// Enable or disable debugging features in Windows and Microsoft C Run Time
// headers.
#if defined(ELECTROSLAG_BUILD_DEBUG)
#if !defined(_DEBUG)
#define _DEBUG
#endif
#if !defined(DEBUG)
#define DEBUG
#endif
#else
#if !defined(NDEBUG)
#define NDEBUG
#endif
#endif

// CRT debugging features.
#if defined(ELECTROSLAG_BUILD_DEBUG) & defined(_MSC_VER)
#include <crtdbg.h>
#endif

// Compiler Intrinsics
#if defined(_MSC_VER)
#include <intrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

// Windows SDK
#include "electroslag/windows_sdk.hpp"
#include "VersionHelpers.h"
#include <mmsystem.h>

// C Run Time
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if defined(_MSC_VER)
#include <process.h>
#endif

// C++ STL
#include <type_traits>
#include <typeinfo>
#include <atomic>
#include <memory>
#include <exception>
#include <stdexcept>
#include <utility>
#include <string>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <deque>
#include <algorithm>

#if defined(_MSC_VER)
#include <filesystem>
namespace std {
    namespace filesystem = ::std::tr2::sys;
}
#else
#include <experimental/filesystem>
namespace std {
    namespace fileystem = ::std::experimental::filesystem::v1;
}
#endif

// OpenGL
#include "electroslag/graphics/glloadgen/gl_core_4_5.hpp"
#if defined(_WIN32)
#include "electroslag/graphics/glloadgen/wgl_core_4_5.hpp"
#endif

// Core electroslag classes that are now static enough to add to the pch
// OR are compiled into other libraries.
#include "electroslag/exception.hpp"
#include "electroslag/utility.hpp"
#include "electroslag/referenced_object.hpp"
#include "electroslag/reference.hpp"

// GLM
#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4324) // warning C4324: structure was padded due to __declspec(align())
#endif
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_AVX
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "electroslag/math/glm_overload.hpp"
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

// GLI
#if !defined(ELECTROSLAG_BUILD_SHIP)
#include "gli/gli.hpp"
#include "gli/convert.hpp"
#endif

// RapidJSON
#if !defined(ELECTROSLAG_BUILD_SHIP)
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_ASSERT(condition) ELECTROSLAG_CHECK(condition)
#define RAPIDJSON_STATIC_ASSERT(condition) ELECTROSLAG_STATIC_CHECK(condition, "RapidJSON static assert")
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#endif

// Visual Leak Debugger
#if defined(_MSC_VER) && defined(ELECTROSLAG_BUILD_DEBUG)
#include <vld.h>
#endif

// Compiler Options
#if defined(_MSC_VER)
#pragma warning (disable : 4324) // warning C4324: structure was padded due to __declspec(align())
#pragma warning (disable : 4505) // warning C4505: unreferenced local function has been removed

// Not cool: the constexpr hash functions generate this warning at the call site in Visual
// C++ 2015, and the only way to suppress it is by shutting off the warning entirely, which
// is potentially dangerous.
// https://connect.microsoft.com/VisualStudio/feedback/details/2636327/warning-c4307-cannot-be-disabled-within-or-immediately-around-a-constexpr-function
#if defined(_MSC_VER) && (_MSC_VER == 1900)
#pragma warning (disable : 4307) // warning C4307 : '*' : integral constant overflow
#endif

namespace std {
    inline int fseeko(FILE* f, long long offset, int whence)
    {
        return (::_fseeki64(f, offset, whence));
    }

    inline long long ftello(FILE* f)
    {
        return (::_ftelli64(f));
    }
}
#endif
