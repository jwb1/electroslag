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
#include "electroslag/serialize/binary_archive.hpp"
#include "electroslag/serialize/serializable_object_interface.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace serialize {
        binary_archive_reader::binary_archive_reader(
            stream_interface* s
            )
            : binary_archive(s)
        {
            header h;
            m_stream->read(&h, sizeof(h));
            if (h.magic_number != magic_number) {
                throw std::runtime_error("binary archive does not have matching magic number");
            }
            if (h.version_number != binary_version_number) {
                throw std::runtime_error("binary archive has unknown version number");
            }
            if (h.object_count <= 0) {
                throw std::runtime_error("binary archive has invalid object count");
            }

            m_current_object = 0;
            m_object_count = h.object_count;
        }

        bool binary_archive_reader::next_object(
            unsigned long long* out_type_hash,
            unsigned long long* out_name_hash
            )
        {
            if (m_current_object < m_object_count) {
                read_uint64(std::string(), out_type_hash);
                read_uint64(std::string(), out_name_hash);
                m_current_object++;
                return (true);
            }
            else {
                return (false);
            }
        }

        bool binary_archive_reader::read_buffer(std::string const&, void* buffer, int sizeof_buffer)
        {
            m_stream->read(buffer, sizeof_buffer);
            return (true);
        }

        bool binary_archive_reader::read_string(std::string const&, std::string* out_value)
        {
            int string_length = 0;
            read_int32(std::string(), &string_length);
            if (string_length > 0) {
                char* s = reinterpret_cast<char*>(alloca(string_length));
                if (read_buffer(std::string(), s, string_length)) {
                    out_value->assign(s);
                    return (true);
                }
            }

            return (false);
        }

        binary_archive_writer::binary_archive_writer(stream_interface* s)
            : binary_archive(s)
        {
            s->seek(sizeof(header), stream_seek_mode_from_start);
        }

        binary_archive_writer::~binary_archive_writer()
        {
            header h;
            h.magic_number = magic_number;
            h.version_number = binary_version_number;
            h.object_count = static_cast<int32_t>(m_saved_objects.size());

            m_stream->seek(0, stream_seek_mode_from_start);
            m_stream->write(&h, sizeof(h));
            m_stream->seek(0, stream_seek_mode_from_end);
        }

        void binary_archive_writer::start_object(serializable_object_interface* obj)
        {
            unsigned long long obj_hash = obj->get_hash();
            ELECTROSLAG_CHECK(obj_hash != 0); // I guess a valid hash of 0 is possible. Seems _highly_ unlikely.

            // Check for duplication; sub-objects might be referred to multiple times.
            saved_object_vector::const_iterator i(m_saved_objects.begin());
            while (i != m_saved_objects.end()) {
                if (*i == obj_hash) {
                    // This object has already been saved; assume this is a dupe!
                    m_write_enable = false;
                    return;
                }
                ++i;
            }

            // Check to see if the object should be filtered due to load key.
            if (m_load_record.is_valid()) {
                if (!m_load_record->has_object(obj)) {
                    m_write_enable = false;
                    return;
                }
            }

            m_write_enable = true;
            m_saved_objects.emplace_back(obj_hash);

            write_uint64(std::string(), obj->get_type_hash());
            write_uint64(std::string(), obj_hash);
        }

        void binary_archive_writer::write_buffer(std::string const&, void const* buffer, int sizeof_buffer)
        {
            if (m_write_enable) {
                m_stream->write(buffer, sizeof_buffer);
            }
        }

        void binary_archive_writer::write_string(std::string const&, std::string const& value)
        {
            write_int32(std::string(), static_cast<int32_t>(value.length() + 1));
            write_buffer(std::string(), value.c_str(), static_cast<int>(value.length() + 1));
        }
    }
}
