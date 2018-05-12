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

#include "electroslag/precomp.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    float random_0_to_1()
    {
        return (std::rand() / (float)RAND_MAX);
    }

    void formatted_string_append(std::string& append_to, char const* format_string, ...)
    {
        va_list arg_list;
        va_start(arg_list, format_string);

        // Figure out how long the formatted string is.
        int formatted_length = std::vsnprintf(0, 0, format_string, arg_list);
        if (formatted_length == -1) {
            throw std::runtime_error("failed to format string.");
        }
        else if (!formatted_length) {
            return;
        }
        else {
            formatted_length++; // +1 for NUL termination
        }

        // Allocated stack space for the string to be appended.
        char* append_string = static_cast<char*>(alloca(formatted_length));
        append_string[0] = '\0';

        // Format the string.
        int return_length = std::vsnprintf(
            append_string,
            formatted_length,
            format_string,
            arg_list
            );
        if (return_length == -1) {
            throw std::runtime_error("failed to format string.");
        }

        va_end(arg_list);

        // Append
        append_to.append(append_string);
    }

    bool being_debugged()
    {
        return (
#if !defined(ELECTROSLAG_BUILD_SHIP) && defined(_WIN32)
            IsDebuggerPresent() ||
#endif
            false
            );
    }
}
