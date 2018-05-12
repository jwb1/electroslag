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

/*
Copyright (C) 2007 Coolsoft Company. All rights reserved.

http://www.coolsoft-sd.com/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

* The origin of this software must not be misrepresented; you must not claim that
you wrote the original software. If you use this software in a product, an
acknowledgment in the product documentation would be appreciated but is not required.
* Altered source versions must be plainly marked as such, and must not be misrepresented
as being the original software.
* This notice may not be removed or altered from any source distribution.
*/

#pragma once

#if defined(ELECTROSLAG_BUILD_SHIP)
#error JSON archiver not to be included in SHIP build!
#endif

namespace electroslag {
    namespace serialize {
        class base64 {
        public:
            static int get_decoded_length(int str_length);
            static int get_enoded_length(int sizeof_buffer);

            static int encode(void const* buffer, int sizeof_buffer, std::string& encoded_string);
            static int decode(char const* str, int str_length, void* buffer, int sizeof_buffer);

        private:
            // Padding character
            static char const pad_char = '[';

            // Disallowed operations:
            base64();
            ~base64();
            explicit base64(base64 const&);
            base64& operator =(base64 const&);
        };
    }
}
