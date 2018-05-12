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
#include "electroslag/file_stream.hpp"

namespace electroslag {

    void file_stream::close()
    {
        flush();

        m_is_open = false;

        m_path.clear();

        if (m_file) {
            std::fclose(m_file);
            m_file = 0;
        }

        if (m_buffer) {
            delete[] m_buffer;
            m_buffer = 0;
        }

        m_access_mode = file_stream_access_mode_default;
        m_cache_mode = file_stream_cache_mode_default;
        m_buffer_size = 0;
        m_buffer_offset = 0;
    }

    void file_stream::read(void* buffer, long long size)
    {
        ELECTROSLAG_CHECK(is_open());
        ELECTROSLAG_CHECK(m_access_mode != file_stream_access_mode_write);
        ELECTROSLAG_CHECK(size > 0);

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            ELECTROSLAG_CHECK(m_buffer_size - m_buffer_offset >= size);
            memcpy(buffer, static_cast<byte*>(m_buffer) + m_buffer_offset, size);
            m_buffer_offset += size;
        }
        else if (m_cache_mode == file_stream_cache_mode_default) {
            if (std::fread(buffer, 1, size, m_file) != static_cast<size_t>(size)) {
                throw std::runtime_error("File read failed");
            }
        }
        else {
            throw std::logic_error("Unknown cache mode");
        }
    }

    void file_stream::write(void const* buffer, long long size)
    {
        ELECTROSLAG_CHECK(is_open());
        ELECTROSLAG_CHECK(m_access_mode != file_stream_access_mode_read);
        ELECTROSLAG_CHECK(size > 0);

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            ELECTROSLAG_CHECK(m_buffer_size - m_buffer_offset >= size);
            memcpy(static_cast<byte*>(m_buffer) + m_buffer_offset, buffer, size);
            m_buffer_offset += size;
        }
        else if (m_cache_mode == file_stream_cache_mode_default) {
            if (std::fwrite(buffer, 1, size, m_file) != static_cast<size_t>(size)) {
                throw std::runtime_error("File write failed");
            }
        }
        else {
            throw std::logic_error("Unknown cache mode");
        }
    }

    void file_stream::flush()
    {
        ELECTROSLAG_CHECK(is_open());

        // Nothing to do if we're just reading.
        if (m_access_mode == file_stream_access_mode_read) return;

        // If we have the file buffered, write the buffer back.
        FILE* write_back_file = 0;
        FILE* flush_file = 0;

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            write_back_file = std::fopen(m_path.c_str(), "wb");
            if (!write_back_file) {
                throw std::runtime_error("file open failure");
            }

            if (std::fwrite(m_buffer, 1, m_buffer_size, write_back_file) != static_cast<size_t>(m_buffer_size)) {
                throw std::runtime_error("File write failed");
            }

            flush_file = write_back_file;
        }
        else if (m_cache_mode == file_stream_cache_mode_default) {
            flush_file = m_file;
        }

        if (std::fflush(flush_file) == EOF) {
            throw std::runtime_error("File flush failed");
        }

        // Don't need to keep around the write back handle.
        if (m_cache_mode == file_stream_cache_mode_buffered) {
            std::fclose(write_back_file);
        }
    }

    void file_stream::seek(long long offset, stream_seek_mode mode /*= stream_seek_mode_from_position */)
    {
        ELECTROSLAG_CHECK(is_open());

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            long long int new_offset = 0;

            if (mode == stream_seek_mode_from_position) {
                new_offset = m_buffer_offset + offset;
            }
            else if (mode == stream_seek_mode_from_start) {
                new_offset = offset;
            }
            else if (mode == stream_seek_mode_from_end) {
                new_offset = (m_buffer_size - 1) + offset;
            }
            else {
                throw parameter_failure("mode");
            }

            ELECTROSLAG_CHECK(new_offset >= 0 && new_offset < m_buffer_size);
            m_buffer_offset = new_offset;
        }
        else {
            int move_origin = 0;

            if (mode == stream_seek_mode_from_position) {
                move_origin = SEEK_CUR;
            }
            else if (mode == stream_seek_mode_from_start) {
                move_origin = SEEK_SET;
            }
            else if (mode == stream_seek_mode_from_end) {
                move_origin = SEEK_END;
            }
            else {
                throw parameter_failure("mode");
            }

            if (std::fseeko(m_file, offset, move_origin) != 0) {
                throw std::runtime_error("File seek failed");
            }
        }
    }

    long long file_stream::get_position() const
    {
        ELECTROSLAG_CHECK(is_open());

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            return (m_buffer_offset);
        }
        else {
            long long position = std::ftello(m_file);
            if (position == -1 && errno > 0) {
                throw std::runtime_error("File position tell failed");
            }
            return (position);
        }
    }

    long long file_stream::get_size() const
    {
        ELECTROSLAG_CHECK(is_open());

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            return (m_buffer_size);
        }
        else {
            long long position = std::ftello(m_file);
            if (position == -1 && errno > 0) {
                throw std::runtime_error("File position tell failed");
            }

            if (std::fseeko(m_file, 0, SEEK_END) != 0) {
                throw std::runtime_error("File seek failed");
            }

            long long size = std::ftello(m_file);
            if (size == -1 && errno > 0) {
                throw std::runtime_error("File position tell failed");
            }

            if (std::fseeko(m_file, position, SEEK_SET) != 0) {
                throw std::runtime_error("File seek failed");
            }

            return (size);
        }
    }

    void file_stream::set_size(long long size)
    {
        ELECTROSLAG_CHECK(is_open());
        ELECTROSLAG_CHECK(size > 0);

        if (m_cache_mode == file_stream_cache_mode_buffered) {
            byte* new_buffer = new byte[size];

            memcpy(new_buffer, m_buffer, min(m_buffer_size, size));
            if (size > m_buffer_size) {
                memset(new_buffer + m_buffer_size, 0, size - m_buffer_size);
            }

            if (m_buffer_offset >= size) {
                m_buffer_offset = 0;
            }

            delete[] m_buffer;
            m_buffer = new_buffer;

            m_buffer_size = size;
        }
        else {
            long long position = std::ftello(m_file);
            if (position == -1 && errno > 0) {
                throw std::runtime_error("File position tell failed");
            }

            if (std::fseeko(m_file, size, SEEK_SET) != 0) {
                throw std::runtime_error("File seek failed");
            }

            if (position < size) {
                if (std::fseeko(m_file, position, SEEK_SET) != 0) {
                    throw std::runtime_error("File seek failed");
                }
            }
        }
    }

    void file_stream::open_helper(std::string const& path, file_stream_access_mode access_mode, file_stream_cache_mode cache_mode, open_helper_mode open_mode)
    {
        ELECTROSLAG_CHECK(!is_open());

        m_path = path;

        char const* mode = 0;
        if (open_mode == open_helper_mode_create_new) {
            switch (access_mode) {
            case file_stream_access_mode_read:
                throw parameter_failure("Can't read newly created file.");

            case file_stream_access_mode_write:
                mode = "wb";
                break;

            case file_stream_access_mode_default:
                mode = "w+b";
                break;

            default:
                throw parameter_failure("access_mode");
            }
        }
        else if (open_mode == open_helper_mode_open_existing) {
            switch (access_mode) {
            case file_stream_access_mode_read:
                mode = "rb";
                break;

            case file_stream_access_mode_write:
            case file_stream_access_mode_default:
                mode = "r+b";
                break;

            default:
                throw parameter_failure("access_mode");
            }
        }
        else {
            throw parameter_failure("open_mode");
        }

        // Open the file.
        m_file = std::fopen(m_path.c_str(), mode);
        if (!m_file) {
            throw std::runtime_error("file open failure");
        }

        // Allocate and read the file for the buffered mode.
        if (cache_mode == file_stream_cache_mode_buffered) {
            if (open_mode == open_helper_mode_open_existing) {
                // Figure out how big the file is in bytes.
                if (std::fseek(m_file, 0, SEEK_END) != 0) {
                    throw std::runtime_error("File seek failed");
                }

                m_buffer_size = std::ftello(m_file);
                if (m_buffer_size == -1 && errno > 0) {
                    throw std::runtime_error("File position tell failed");
                }

                if (std::fseek(m_file, 0, SEEK_SET) != 0) {
                    throw std::runtime_error("File seek failed");
                }

                // Allocate.
                m_buffer = new byte[m_buffer_size];
                m_buffer_offset = 0;

                // Read the file.
                if (std::fread(m_buffer, 1, m_buffer_size, m_file) != static_cast<size_t>(m_buffer_size)) {
                    throw std::runtime_error("File read failed");
                }
            }
            else if (open_mode == open_helper_mode_create_new) {
                // Nothing in new file; set_size to get started.
                m_buffer = 0;
                m_buffer_size = 0;
                m_buffer_offset = 0;
            }

            std::fclose(m_file);
            m_file = 0;
        }

        // Store misc. details.
        m_access_mode = access_mode;
        m_cache_mode = cache_mode;
        m_is_open = true;
    }
}
