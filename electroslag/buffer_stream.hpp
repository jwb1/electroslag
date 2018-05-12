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
#include "electroslag/referenced_buffer.hpp"

namespace electroslag {
    class buffer_stream : public stream_interface {
    public:
        explicit buffer_stream(referenced_buffer_interface::ref const& buffer_ref)
            : m_buffer_accessor(buffer_ref)
            , m_buffer_offset(0)
        {}

        virtual void read(void* buffer, long long size)
        {
            ELECTROSLAG_CHECK(size > 0);
            ELECTROSLAG_CHECK(m_buffer_accessor.get_sizeof() - m_buffer_offset >= size);

            memcpy(buffer, static_cast<byte*>(m_buffer_accessor.get_pointer()) + m_buffer_offset, size);
            m_buffer_offset += size;
        }

        virtual void write(void const* buffer, long long size)
        {
            ELECTROSLAG_CHECK(size > 0);
            ELECTROSLAG_CHECK(m_buffer_accessor.get_sizeof() - m_buffer_offset >= size);
            memcpy(static_cast<byte*>(m_buffer_accessor.get_pointer()) + m_buffer_offset, buffer, size);
            m_buffer_offset += size;
        }

        virtual void flush()
        {}

        virtual void seek(
            long long offset,
            stream_seek_mode mode = stream_seek_mode_from_position
            )
        {
            long long int new_offset = 0;

            if (mode == stream_seek_mode_from_position) {
                new_offset = m_buffer_offset + offset;
            }
            else if (mode == stream_seek_mode_from_start) {
                new_offset = offset;
            }
            else if (mode == stream_seek_mode_from_end) {
                new_offset = (m_buffer_accessor.get_sizeof() - 1) + offset;
            }
            else {
                throw parameter_failure("mode");
            }

            ELECTROSLAG_CHECK(new_offset >= 0 && new_offset < m_buffer_accessor.get_sizeof());
            m_buffer_offset = new_offset;
        }

        virtual long long get_position() const
        {
            return (m_buffer_offset);
        }

        virtual long long get_size() const
        {
            return (m_buffer_accessor.get_sizeof());
        }

        virtual void set_size(long long)
        {
            throw std::logic_error("It is not possible to change a buffer streams size");
        }

    private:
        referenced_buffer_interface::accessor m_buffer_accessor;
        long long m_buffer_offset;
    };
}
