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
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/graphics/vertex_attribute.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace graphics {
        class primitive_stream_descriptor
            : public referenced_object
            , public serialize::serializable_object<primitive_stream_descriptor> {
        public:
            typedef reference<primitive_stream_descriptor> ref;
            typedef std::vector<vertex_attribute*> vertex_attribute_vector;
            typedef vertex_attribute_vector::const_iterator const_attribute_iterator;
            typedef vertex_attribute_vector::iterator attribute_iterator;

            static ref create()
            {
                return (ref(new primitive_stream_descriptor()));
            }

            // Implement serializable_object
            explicit primitive_stream_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            primitive_type get_prim_type() const
            {
                return (m_prim_type);
            }

            void set_prim_type(primitive_type prim_type)
            {
                ELECTROSLAG_CHECK(prim_type > primitive_type_unknown && prim_type < primitive_type_count);
                m_prim_type = prim_type;
            }

            int get_prim_count() const
            {
                return (m_prim_count);
            }

            void set_prim_count(int prim_count)
            {
                ELECTROSLAG_CHECK(prim_count > 0);
                m_prim_count = prim_count;
            }

            int get_sizeof_index() const
            {
                return (m_sizeof_index);
            }

            buffer_descriptor::ref const& get_index_buffer() const
            {
                return (m_index_buffer);
            }

            buffer_descriptor::ref& get_index_buffer()
            {
                return (m_index_buffer);
            }

            void set_index_buffer(int sizeof_index, buffer_descriptor::ref const& index_buffer)
            {
                ELECTROSLAG_CHECK(sizeof_index > 0);
                ELECTROSLAG_CHECK(index_buffer.is_valid());

                m_sizeof_index = sizeof_index;
                m_index_buffer = index_buffer;
            }

            // Store the vertex metadata two ways:
            // - The field map allows shaders to "look up" field binding points
            //   This allows a shader to specify that a field should be "position" without
            //   knowing the index.
            // - The attribute list allows the VAO itself to be created.
            shader_field_map::ref const& get_fields() const
            {
                return (m_fields);
            }

            shader_field_map::ref& get_fields()
            {
                return (m_fields);
            }

            void set_fields(shader_field_map::ref& fields)
            {
                m_fields = fields;
            }

            void insert_vertex_attribute(
                vertex_attribute* attrib
                )
            {
                m_vertex_attributes.emplace_back(attrib);
            }

            const_attribute_iterator begin_attributes() const
            {
                return (m_vertex_attributes.begin());
            }

            attribute_iterator begin_attributes()
            {
                return (m_vertex_attributes.begin());
            }

            const_attribute_iterator end_attributes() const
            {
                return (m_vertex_attributes.end());
            }

            attribute_iterator end_attributes()
            {
                return (m_vertex_attributes.end());
            }

            int get_attribute_count() const
            {
                return (static_cast<int>(m_vertex_attributes.size()));
            }

        private:
            primitive_stream_descriptor()
                : m_prim_type(primitive_type_unknown)
                , m_prim_count(-1)
                , m_sizeof_index(-1)
            {}

#if !defined(ELECTROSLAG_BUILD_SHIP)
            void verify_fields_and_attribs();
#endif

            primitive_type m_prim_type;
            int m_prim_count;

            shader_field_map::ref m_fields;
            vertex_attribute_vector m_vertex_attributes;

            int m_sizeof_index;
            buffer_descriptor::ref m_index_buffer;

            // Disallowed operations:
            explicit primitive_stream_descriptor(primitive_stream_descriptor const&);
            primitive_stream_descriptor& operator =(primitive_stream_descriptor const&);
        };
    }
}
