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

namespace electroslag {
    enum stream_seek_mode {
        stream_seek_mode_invalid = -1,
        stream_seek_mode_from_position,
        stream_seek_mode_from_start,
        stream_seek_mode_from_end
    };

    class stream_interface {
    public:
        virtual void read(void* buffer, long long size) = 0;
        virtual void write(void const* buffer, long long size) = 0;
        virtual void flush() = 0;
        virtual void seek(
            long long offset,
            stream_seek_mode mode = stream_seek_mode_from_position
            ) = 0;
        virtual long long get_position() const = 0;
        virtual long long get_size() const = 0;
        virtual void set_size(long long size) = 0;
    };
}
