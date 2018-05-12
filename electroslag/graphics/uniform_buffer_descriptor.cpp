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
#include "electroslag/graphics/uniform_buffer_descriptor.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        uniform_buffer_descriptor::uniform_buffer_descriptor(serialize::archive_reader_interface* ar)
            : m_binding(-1)
            , m_size(-1)
        {
            unsigned long long field_map_hash = 0;
            if (!ar->read_name_hash("field_map", &field_map_hash)) {
                throw load_object_failure("field_map");
            }

            m_fields = serialize::get_database()->find_object_ref<shader_field_map>(field_map_hash);
        }

        void uniform_buffer_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            m_fields->save_to_archive(ar);

            // Write in the ubo description
            serializable_object::save_to_archive(ar);
            ar->write_name_hash("field_map", m_fields->get_hash());
        }
    }
}
