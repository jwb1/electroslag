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

// The OS malloc implementation will ensure all returned pointers are aligned to at least this.
// Windows minimum alignment documented to be 16 here:
// https://msdn.microsoft.com/en-us/library/6ewkz86d.aspx
#if defined(_MSC_VER)
#if defined(_WIN64)
#define ELECTROSLAG_NATURAL_HEAP_ALIGN 16
#elif defined(_WIN32)
#define ELECTROSLAG_NATURAL_HEAP_ALIGN 8
#endif
#elif defined(__GNUC__)
#define ELECTROSLAG_NATURAL_HEAP_ALIGN __BIGGEST_ALIGNMENT__
#endif

#if !defined(ELECTROSLAG_NATURAL_HEAP_ALIGN)
#error Cannot determine natural heap alignment.
#endif

// This macro is used for initializing named objects. In SHIP builds, don't
// keep the string around, just the hash.
#if !defined(ELECTROSLAG_BUILD_SHIP)
#define ELECTROSLAG_STRING_AND_HASH(s) s, ::electroslag::hash_string(s)
#else
#define ELECTROSLAG_STRING_AND_HASH(s) ::electroslag::hash_string(s)
#endif

#define ELECTROSLAG_STRING_AND_LENGTH(s) s, ::electroslag::length_string(s)

namespace electroslag {
    typedef unsigned char byte;

    // This union can be used to access the individual fields of a single
    // precision floating point value. The number of bits are specified in the
    // IEEE standard: http://en.wikipedia.org/wiki/Single_precision
    ELECTROSLAG_STATIC_CHECK(sizeof(uint32_t) == sizeof(float), "Float type is not 32 bits?");
    union float_fields {
        float val;
        uint32_t i_val;
        struct {
            uint32_t mantissa:23;
            uint32_t exponent:8;
            uint32_t sign:1;
        } fields;
    };

    // This union can be used to access the individual fields of a double
    // precision floating point value. The number of bits are specified in the
    // IEEE standard: http://en.wikipedia.org/wiki/Double_precision
    ELECTROSLAG_STATIC_CHECK(sizeof(uint64_t) == sizeof(double), "Double type is not 64 bits?");
    union double_fields {
        double val;
        uint64_t i_val;
        struct {
            uint64_t mantissa:52;
            uint64_t exponent:11;
            uint64_t sign:1;
        } fields;
    };

    template<class T>
    inline T min(T a, T b)
    {
        return ((a < b) ? a : b);
    }

    template<class T>
    inline T max(T a, T b)
    {
        return ((a < b) ? b : a);
    }

    template<class T>
    inline bool is_pow2(T val)
    {
        ELECTROSLAG_STATIC_CHECK((std::is_integral<T>::value),
            "is_pow2 only expands correctly for integral types");
        return (!(val & (val - 1)) && val);
    }

    template<class T>
    inline constexpr bool is_aligned(T value, size_t alignment)
    {
        return ((reinterpret_cast<uintptr_t>(value) % alignment) == 0);
    }

    inline constexpr unsigned int align_up(unsigned int value, unsigned int alignment)
    {
        return ((value + (alignment - 1)) & ~(alignment - 1));
    }

    template <class T>
    inline T* align_up(T* value, unsigned int alignment)
    {
        uintptr_t a = alignment; // Avoid warning C4319: '~': zero extending 'unsigned int' to 'uintptr_t' of greater size
        return (reinterpret_cast<T*>(
            (reinterpret_cast<uintptr_t>(value) + (a - 1)) & ~(a - 1)));
    }

    template<class T, class U>
    inline T next_multiple_of_2(
        T val,
        U multiple_of_2
        )
    {
        ELECTROSLAG_STATIC_CHECK((std::is_integral<T>::value),
            "next_multiple_of_2 only expands correctly for integral types");
        ELECTROSLAG_STATIC_CHECK((std::is_integral<U>::value),
            "next_multiple_of_2 only expands correctly for integral types");
        ELECTROSLAG_CHECK(multiple_of_2 >= 2);
        ELECTROSLAG_CHECK(is_pow2(multiple_of_2));
        // ((~multiple_of_2) + 1) == -multiple_of_2
        return ((val <= 0) ? (multiple_of_2) : (val + (multiple_of_2 - 1)) & ((~multiple_of_2) + 1));
    }

    float random_0_to_1();

    // Append a printf-style formatted string to a C++ string.
    void formatted_string_append(std::string& append_to, char const* format_string, ...);

    // Functor for use in unordered map and set when the key is already a hashed value
    template<class T>
    class prehashed_key{
    public:
        size_t operator ()(T const& prehashed_key_value) const
        {
            return (static_cast<size_t>(prehashed_key_value & SIZE_MAX));
        }
    };

    // Compile time string length
    inline constexpr int length_string(char const* s)
    {
        return ((*s == '\0') ? (0) : (length_string(s + 1) + 1));
    }

    // String hashing
    namespace fnv1a {
        // FNV-1a constants
        static constexpr unsigned long long basis = 14695981039346656037ULL;
        static constexpr unsigned long long prime = 1099511628211ULL;

        // compile-time hash helper function
        inline constexpr unsigned long long hash_one_char(char c, char const* remaining_string, unsigned long long value)
        {
            return ((c == 0) ? (value) : (hash_one_char(remaining_string[0], remaining_string + 1, (value ^ c) * prime)));
        }
    }

    // Compile time hash, hopefully
    inline constexpr unsigned long long hash_string(char const* s)
    {
        return (fnv1a::hash_one_char(s[0], s + 1, fnv1a::basis));
    }

    // Always a run-time hash
    inline unsigned long long hash_string_runtime(char const* s)
    {
        unsigned long long hash = fnv1a::basis;
        while (*s != 0) {
            hash ^= s[0];
            hash *= fnv1a::prime;
            ++s;
        }
        return (hash);
    }

    inline unsigned long long hash_string_runtime(std::string const& s)
    {
        return (hash_string_runtime(s.c_str()));
    }

    // Helpers for running under a debugger.
    bool being_debugged();
}
