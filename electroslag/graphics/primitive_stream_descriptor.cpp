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
#include "electroslag/graphics/primitive_stream_descriptor.hpp"

namespace electroslag {
    namespace graphics {
        primitive_stream_descriptor::primitive_stream_descriptor(serialize::archive_reader_interface* ar)
        {
            if (!ar->read_enumeration("prim_type", &m_prim_type, primitive_type_strings)) {
                throw load_object_failure("prim_type");
            }

            if (!ar->read_int32("prim_count", &m_prim_count)) {
                throw load_object_failure("prim_count");
            }

            if (!ar->read_int32("sizeof_index", &m_sizeof_index)) {
                throw load_object_failure("sizeof_index");
            }

            unsigned long long ibo_name_hash = 0;
            if (!ar->read_name_hash("ibo", &ibo_name_hash)) {
                throw load_object_failure("ibo");
            }
            m_index_buffer = serialize::get_database()->find_object_ref<buffer_descriptor>(ibo_name_hash);

            unsigned long long field_map_hash = 0;
            if (!ar->read_name_hash("field_map", &field_map_hash)) {
                throw load_object_failure("field_map");
            }

            m_fields = serialize::get_database()->find_object_ref<shader_field_map>(field_map_hash);

            int attrib_count = 0;
            if (!ar->read_int32("attrib_count", &attrib_count)) {
                throw load_object_failure("attrib_count");
            }
            m_vertex_attributes.reserve(attrib_count);

            serialize::enumeration_namer namer(attrib_count, 'a');
            while (!namer.used_all_names()) {
                unsigned long long attrib_name_hash = 0;
                if (!ar->read_name_hash(namer.get_next_name(), &attrib_name_hash)) {
                    throw load_object_failure("attrib");
                }
                m_vertex_attributes.emplace_back(
                    dynamic_cast<vertex_attribute*>(serialize::get_database()->find_object(attrib_name_hash))
                    );
            }

#if !defined(ELECTROSLAG_BUILD_SHIP)
            verify_fields_and_attribs();
#endif
        }

        void primitive_stream_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
#if !defined(ELECTROSLAG_BUILD_SHIP)
            verify_fields_and_attribs();
#endif

            m_fields->save_to_archive(ar);

            attribute_iterator i(m_vertex_attributes.begin());
            while (i != m_vertex_attributes.end()) {
                (*i)->save_to_archive(ar);
                ++i;
            }
            m_index_buffer->save_to_archive(ar);

            serializable_object::save_to_archive(ar);

            ar->write_int32("prim_type", m_prim_type);
            ar->write_int32("prim_count", m_prim_count);
            ar->write_int32("sizeof_index", m_sizeof_index);
            ar->write_name_hash("ibo", m_index_buffer->get_hash());
            ar->write_name_hash("field_map", m_fields->get_hash());

            int attrib_count = static_cast<int>(m_vertex_attributes.size());
            ar->write_int32("attrib_count", attrib_count);

            serialize::enumeration_namer namer(attrib_count, 'a');
            i = m_vertex_attributes.begin();
            while (i != m_vertex_attributes.end()) {
                ar->write_name_hash(namer.get_next_name(), (*i)->get_hash());
                ++i;
            }
        }

#if !defined(ELECTROSLAG_BUILD_SHIP)
        void primitive_stream_descriptor::verify_fields_and_attribs()
        {
            // TODO: Sanity tests the incoming shader field map and vertex attribute list.
            // They should be describing the same layout!
        }
#endif
    }
}
