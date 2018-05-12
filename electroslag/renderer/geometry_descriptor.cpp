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
#include "electroslag/renderer/geometry_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        geometry_descriptor::geometry_descriptor(serialize::archive_reader_interface* ar)
        {
            unsigned long long prim_stream_name_hash = 0;
            if (!ar->read_name_hash("prim_stream", &prim_stream_name_hash)) {
                throw load_object_failure("prim_stream");
            }

            m_primitive_stream = serialize::get_database()->find_object_ref<graphics::primitive_stream_descriptor>(
                prim_stream_name_hash
                );

            if (!ar->read_int32("prim_count", &m_element_count) || m_element_count < 0) {
                throw load_object_failure("prim_count");
            }
            if (!ar->read_int32("index_offset", &m_index_buffer_start_offset) || m_index_buffer_start_offset < 0) {
                throw load_object_failure("index_offset");
            }
            if (!ar->read_int32("index_value_offset", &m_index_value_offset)) {
                throw load_object_failure("index_value_offset");
            }
            if (!ar->read_buffer("aabb", &m_aabb, sizeof(m_aabb))) {
                compute_aabb();
            }
        }

        void geometry_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            m_primitive_stream->save_to_archive(ar);

            serializable_object::save_to_archive(ar);

            ar->write_name_hash("prim_stream", m_primitive_stream->get_hash());
            ar->write_int32("prim_count", m_element_count);
            ar->write_int32("index_offset", m_index_buffer_start_offset);
            ar->write_int32("index_value_offset", m_index_value_offset);
            ar->write_buffer("aabb", &m_aabb, sizeof(m_aabb));
        }

        void geometry_descriptor::compute_aabb()
        {
#if !defined(ELECTROSLAG_BUILD_SHIP)
            // Find the position shader field.
            graphics::shader_field const* field = 0;
            graphics::shader_field_map::ref const& field_map(m_primitive_stream->get_fields());
            graphics::shader_field_map::const_iterator f(field_map->begin());
            while (f != field_map->end()) {
                field = f->second;
                if (field->get_kind() == graphics::field_kind::field_kind_attribute_position) {
                    break;
                }
                ++f;
            }
            if (f == field_map->end()) {
                throw load_object_failure("Cannot compute aabb without vertex positions");
            }

            // Find the position vertex attribute attached to the field.
            graphics::vertex_attribute const* attrib = 0;
            graphics::primitive_stream_descriptor::const_attribute_iterator a(m_primitive_stream->begin_attributes());
            while (a != m_primitive_stream->end_attributes()) {
                attrib = *a;
                if (*attrib->get_field() == *field) {
                    break;
                }
                ++a;
            }
            if (a == m_primitive_stream->end_attributes()) {
                throw load_object_failure("Cannot compute aabb without vertex positions");
            }

            // Get access to the VBO and IBO for the primitive stream.
            graphics::buffer_descriptor::ref ibo_desc(m_primitive_stream->get_index_buffer());
            ELECTROSLAG_CHECK(ibo_desc->has_initialized_data());
            referenced_buffer_interface::ref const& ibo(ibo_desc->get_initialized_data());

            graphics::buffer_descriptor::ref vbo_desc(attrib->get_buffer());
            ELECTROSLAG_CHECK(vbo_desc->has_initialized_data());
            referenced_buffer_interface::ref const& vbo(vbo_desc->get_initialized_data());

            {
                referenced_buffer_interface::accessor index_accessor(ibo);
                referenced_buffer_interface::accessor vertex_accessor(vbo);

                void const* index_data = index_accessor.get_pointer();
                void const* vertex_data = vertex_accessor.get_pointer();

                // Compute the number of "elements" we need to look at; basically an element
                // is a vertex looked up by index, in this case.
                int element_count = m_element_count;
                if (element_count <= 0) {
                    element_count = graphics::primitive_type_util::get_element_count(
                        m_primitive_stream->get_prim_type(),
                        m_primitive_stream->get_prim_count()
                        );
                }

                // Read out the indexes.
                for (int i = 0; i < element_count; ++i) {
                    int index;
                    switch (m_primitive_stream->get_sizeof_index()) {
                    case 1:
                        index = *(static_cast<int8_t const*>(index_data) + m_index_buffer_start_offset);
                        break;

                    case 2:
                        index = *(static_cast<int16_t const*>(index_data) + m_index_buffer_start_offset);
                        break;

                    case 4:
                        index = *(static_cast<int32_t const*>(index_data) + m_index_buffer_start_offset);
                        break;

                    default:
                        throw load_object_failure("Cannot generate aabb with unknown index size");
                    }
                    index += m_index_value_offset;

                    // Compute the position address from the index and incorporate it in the aabb.
                    glm::f32vec3 const* position = reinterpret_cast<glm::f32vec3 const*>(
                        static_cast<byte const*>(vertex_data) + (attrib->get_stride() * index) + field->get_offset()
                        );
                    m_aabb.add_vert(*position);
                }
            }
#else
            throw load_object_failure("aabb");
#endif
        }
    }
}
