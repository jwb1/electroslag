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
#include "electroslag/graphics/buffer_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        buffer_descriptor::buffer_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_enumeration("buffer_memory_map", &m_memory_map, buffer_memory_map_strings)) {
                throw load_object_failure("buffer_memory_map");
            }

            if (!ar->read_enumeration("buffer_memory_caching", &m_memory_caching, buffer_memory_caching_strings)) {
                throw load_object_failure("buffer_memory_caching");
            }

            int data_sizeof = 0;
            if (!ar->read_int32("data_sizeof", &data_sizeof)) {
                throw load_object_failure("data_sizeof");
            }
            m_data = referenced_buffer_from_sizeof::create(data_sizeof);
            {
                referenced_buffer_interface::accessor data_accessor(m_data);
                if (!ar->read_buffer("data", data_accessor.get_pointer(), data_sizeof)) {
                    throw load_object_failure("data");
                }
            }
        }

        void buffer_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            ar->write_int32("buffer_memory_map", m_memory_map);
            ar->write_int32("buffer_memory_caching", m_memory_caching);

            {
                referenced_buffer_interface::accessor data_accessor(m_data);
                int data_sizeof = data_accessor.get_sizeof();
                ar->write_int32("data_sizeof", data_sizeof);
                ar->write_buffer("data", data_accessor.get_pointer(), data_sizeof);
            }
        }
    }
}
