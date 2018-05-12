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

#include "electroslag/precomp.hpp"
#include "electroslag/serialize/base64.hpp"

namespace electroslag {
    namespace serialize {
        // static
        int base64::get_decoded_length(int str_length)
        {
            // Returns maximum size of decoded data based on size of base64 encoded string.
            return (str_length - (str_length / 4));
        }

        // static
        int base64::get_enoded_length(int sizeof_buffer)
        {
            // Returns maximum length of base64 encoded string based on size of uncoded data.
            int encoded_length = sizeof_buffer + (sizeof_buffer / 3) + (((sizeof_buffer % 3) != 0) ? 1 : 0);

            // Encoded size must be multiple of 4 bytes
            int length_mod_4 = encoded_length % 4;
            if (length_mod_4 != 0) {
                encoded_length += (4 - length_mod_4);
            }

            return (encoded_length);
        }

        // static
        int base64::encode(void const* buffer, int sizeof_buffer, std::string& encoded_string)
        {
            static char const character_lookup[] = {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '-'
            };

            // Mask with low six bits of a word set.
            static int const six_bit_mask = 0x3F;

            // used as temp 24-bits buffer
            union {
                byte bytes[4];
                unsigned int block;
            } temp_buffer;
            temp_buffer.block = 0;

            byte const* input_buffer = static_cast<byte const*>(buffer);

            // output buffer which holds code during conversation
            int encoded_length = get_enoded_length(sizeof_buffer);
            encoded_string.clear();
            encoded_string.reserve(encoded_length);

            // Conversion is done by taking three bytes at time of input data int temp
            // then four six-bits values are extracted, converted to base64 characters
            // and at the end they are written to output buffer
            for (int i = 0, bytes_left = sizeof_buffer; i < sizeof_buffer; i += 3, bytes_left -= 3) {
                // Filling temp buffer

                // Get first byte and puts it at MSB position in temp buffer
                temp_buffer.bytes[2] = input_buffer[i];

                // more data left?
                if (bytes_left > 1) {
                    // get second byte and puts it at middle position in temp buffer
                    temp_buffer.bytes[1] = input_buffer[i + 1];
                    // more data left?
                    if (bytes_left > 2) {
                        // get third byte and puts it at LSB position in temp buffer
                        temp_buffer.bytes[0] = input_buffer[i + 2];
                    }
                    else {
                        // zero-padding of input data (last bytes)
                        temp_buffer.bytes[0] = 0;
                    }
                }
                else {
                    // zero-padding of input data (last two bytes)
                    temp_buffer.bytes[1] = 0;
                    temp_buffer.bytes[0] = 0;
                }

                // Constructing encoded characters from temp buffer and append to the
                // output string

                // Extract first and second six-bit value from temp buffer
                // and convert is to base64 character
                encoded_string.push_back(character_lookup[(temp_buffer.block >> 18) & six_bit_mask]);
                encoded_string.push_back(character_lookup[(temp_buffer.block >> 12) & six_bit_mask]);
                // more data left?
                if (bytes_left > 1) {
                    // extract third six-bit value from temp buffer
                    // and convert it to base64 character
                    encoded_string.push_back(character_lookup[(temp_buffer.block >> 6) & six_bit_mask]);
                    // more data left?
                    if (bytes_left > 2) {
                        // extract forth six-bit value from temp buffer
                        // and convert it to base64 character
                        encoded_string.push_back(character_lookup[temp_buffer.block & six_bit_mask]);
                    }
                    else {
                        // pad output code
                        encoded_string.push_back(pad_char);
                    }
                }
                else {
                    // pad output code
                    encoded_string.push_back(pad_char);
                    encoded_string.push_back(pad_char);
                }
            }

            return (encoded_length);
        }

        // static
        int base64::decode(char const* str, int str_length, void* buffer, int sizeof_buffer)
        {
            // used as temp 24-bits buffer
            union {
                byte bytes[4];
                unsigned int block;
            } temp_buffer;
            temp_buffer.block = 0;

            byte* output_buffer = static_cast<byte*>(buffer);

            // number of decoded bytes
            int j = 0;

            for (int i = 0; i < str_length; i++) {
                // position in temp buffer
                int m = i % 4;
                int val = 0;
                char current_char = str[i];

                // converts base64 character to six-bit value
                if (current_char >= 'A' && current_char <= 'Z') {
                    val = current_char - 'A';
                }
                else if (current_char >= 'a' && current_char <= 'z') {
                    val = current_char - 'a' + 'Z' - 'A' + 1;
                }
                else if (current_char >= '0' && current_char <= '9') {
                    val = current_char - '0' + ('Z' - 'A' + 1) * 2;
                }
                else if (current_char == '*') {
                    val = 62;
                }
                else if (current_char == '-') {
                    val = 63;
                }

                // padding chars are not decoded and written to output buffer
                if (current_char != pad_char) {
                    temp_buffer.block |= val << (3 - m) * 6;
                }
                else {
                    m--;
                }

                // temp buffer is full or end of code is reached
                // flushing temp buffer
                if (m == 3 || current_char == pad_char) {
                    // writes byte from temp buffer (combined from two six-bit values) to output buffer
                    if (j < sizeof_buffer) {
                        output_buffer[j++] = temp_buffer.bytes[2];
                    }
                    else {
                        break;
                    }

                    // more data left?
                    if (current_char != pad_char || m > 1) {
                        // writes byte from temp buffer (combined from two six-bit values) to output buffer
                        if (j < sizeof_buffer) {
                            output_buffer[j++] = temp_buffer.bytes[1];
                        }
                        else {
                            break;
                        }

                        // more data left?
                        if (current_char != pad_char || m > 2) {
                            // writes byte from temp buffer (combined from two six-bit values) to output buffer
                            if (j < sizeof_buffer) {
                                output_buffer[j++] = temp_buffer.bytes[0];
                            }
                            else {
                                break;
                            }
                        }
                    }

                    // restarts temp buffer
                    temp_buffer.block = 0;
                }

                // when padding char is reached it is the end of code
                if (current_char == pad_char) {
                    break;
                }
            }

            return (j);
        }
    }
}
