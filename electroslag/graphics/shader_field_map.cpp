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
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        shader_field_map::shader_field_map(serialize::archive_reader_interface* ar)
        {
            int field_count = 0;
            if (!ar->read_int32("field_count", &field_count) || field_count <= 0) {
                throw load_object_failure("field_count");
            }
            m_map.reserve(field_count);

            serialize::enumeration_namer namer(field_count, 'f');
            while (!namer.used_all_names()) {
                unsigned long long field_hash = 0;
                if (!ar->read_name_hash(namer.get_next_name(), &field_hash)) {
                    throw load_object_failure("field");
                }

                shader_field* field = dynamic_cast<shader_field*>(serialize::get_database()->find_object(field_hash));
                insert(field);
            }
        }

        void shader_field_map::save_to_archive(serialize::archive_writer_interface* ar)
        {
            // Write in all of the fields in the map.
            iterator i(begin());
            while (i != end()) {
                i->second->save_to_archive(ar);
                ++i;
            }

            // Write in the map listing
            serializable_object::save_to_archive(ar);

            int field_count = get_field_count();
            ar->write_int32("field_count", field_count);

            serialize::enumeration_namer namer(field_count, 'f');
            i = begin();
            while (i != end()) {
                ar->write_name_hash(namer.get_next_name(), i->first);
                ++i;
            }
        }

        int shader_field_map::get_fields_total_size() const
        {
            int total_size = 0;

            const_iterator i(begin());
            while (i != end()) {
                total_size += field_type_util::get_bytes(i->second->get_field_type());
                ++i;
            }

            return (total_size);
        }
    }
}
