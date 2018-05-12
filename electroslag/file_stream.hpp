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
#include "electroslag/stream_interface.hpp"

namespace electroslag {
    enum file_stream_cache_mode {
        file_stream_cache_mode_invalid = -1,
        file_stream_cache_mode_default, // Use OS buffering
        file_stream_cache_mode_buffered, // Read the whole file into RAM.
    };

    enum file_stream_access_mode {
        file_stream_access_mode_invalid = -1,
        file_stream_access_mode_read = 1,
        file_stream_access_mode_write = 2,
        file_stream_access_mode_default = 3 // Read and write
    };

    class file_stream : public stream_interface {
    public:
        file_stream() :
            m_file(0),
            m_access_mode(file_stream_access_mode_default),
            m_cache_mode(file_stream_cache_mode_default),
            m_buffer(0),
            m_buffer_size(0),
            m_buffer_offset(0),
            m_is_open(false)
        {}

        virtual ~file_stream()
        {
            if (is_open()) {
                close();
            }
        }

        void create_new(
            std::string const& path,
            file_stream_access_mode access_mode = file_stream_access_mode_default,
            file_stream_cache_mode cache_mode = file_stream_cache_mode_default
            )
        {
            open_helper(path, access_mode, cache_mode, open_helper_mode_create_new);
        }

        void open(
            std::string const& path,
            file_stream_access_mode access_mode = file_stream_access_mode_default,
            file_stream_cache_mode cache_mode = file_stream_cache_mode_default
            )
        {
            open_helper(path, access_mode, cache_mode, open_helper_mode_open_existing);
        }

        void close();

        bool is_open() const
        {
            return (m_is_open);
        }

        // Implement stream_interface
        virtual void read(void* buffer, long long size);
        virtual void write(void const* buffer, long long size);
        virtual void flush();

        virtual void seek(
            long long offset,
            stream_seek_mode mode = stream_seek_mode_from_position
            );

        virtual long long get_position() const;
        virtual long long get_size() const;
        virtual void set_size(long long size);

    protected:
        enum open_helper_mode {
            open_helper_mode_create_new,
            open_helper_mode_open_existing
        };

        void open_helper(
            std::string const& path,
            file_stream_access_mode access_mode,
            file_stream_cache_mode cache_mode,
            open_helper_mode open_mode
            );

        std::string m_path;
        FILE* m_file;

        file_stream_access_mode m_access_mode;
        file_stream_cache_mode m_cache_mode;

        void* m_buffer;
        long long m_buffer_size;
        long long m_buffer_offset;

        bool m_is_open;

        // Disallowed operations:
        explicit file_stream(file_stream const&);
        file_stream& operator =(file_stream const&);
    };
}
