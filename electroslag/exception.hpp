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

// Compile time condition check
#define ELECTROSLAG_STATIC_CHECK(condition, description) \
    static_assert(condition, description)

// Run time condition check
#if !defined(ELECTROSLAG_BUILD_SHIP)
#define ELECTROSLAG_FAIL(condition) throw ::electroslag::check_failure(condition)

#define ELECTROSLAG_CHECK(condition) \
    (void)( (!!(condition)) ? \
            (void)(0) : \
            throw ::electroslag::check_failure(#condition) \
          )
#else
#define ELECTROSLAG_FAIL(condition) (void)(0)
#define ELECTROSLAG_CHECK(condition) (void)(0)
#endif

// Compile time check an enum will fit in a number of bits. To use:
// Use "-1" to reserve a bit pattern for the _unknown = -1 enum value and ensure
// there is a _count enum value at the end of the enumeration.
// This currently allows _unknown == _count in packed form. (Using >=)
#define ELECTROSLAG_ENUM_BITPACK_CHECK(enum_name, bits) \
    ELECTROSLAG_STATIC_CHECK((((0x1 << bits) - 1) >= enum_name##_count), "Enum bitpack check")

namespace electroslag {
    // Exceptions are loosely characterized by a derived types.
    class parameter_failure : public std::logic_error {
    public:
        explicit parameter_failure(char const* parameter) throw()
            : std::logic_error(parameter)
        {}
    };

#if !defined(ELECTROSLAG_BUILD_SHIP)
    class check_failure : public std::logic_error {
    public:
        explicit check_failure(char const* condition) throw()
            : std::logic_error(condition)
        {}
    };
#endif

#if defined(_WIN32)
    class win32_api_failure : public std::runtime_error {
    public:
        explicit win32_api_failure(char const* description) throw()
            : std::runtime_error(description)
            , m_last_error(GetLastError())
        {}

    private:
        unsigned int m_last_error;
    };
#endif

    class opengl_api_failure : public std::runtime_error {
    public:
        opengl_api_failure(
            char const* description,
            GLenum opengl_error
            ) throw()
            : std::runtime_error(description)
            , m_opengl_error(opengl_error)
        {}

    private:
        GLenum m_opengl_error;
    };

    class object_not_found_failure : public std::logic_error {
    public:
        object_not_found_failure(
            char const* description,
            unsigned long long name_hash
            ) throw()
            : std::logic_error(description)
            , m_name_hash(name_hash)
        {}

    private:
        unsigned long long m_name_hash;
    };

    class load_object_failure : public std::runtime_error {
    public:
        explicit load_object_failure(char const* description) throw()
            : std::runtime_error(description)
        {}
    };

    class timeout_failure : public std::runtime_error {
    public:
        explicit timeout_failure(char const* description) throw()
            : std::runtime_error(description)
        {}
    };
}
