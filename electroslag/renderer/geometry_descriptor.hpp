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
#include "electroslag/math/aabb.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/graphics/primitive_stream_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class geometry_descriptor
            : public referenced_object
            , public serialize::serializable_object<geometry_descriptor> {
        public:
            typedef reference<geometry_descriptor> ref;

            static ref create()
            {
                return (ref(new geometry_descriptor()));
            }

            // Implement serializable_object
            explicit geometry_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            graphics::primitive_stream_descriptor::ref const& get_primitive_stream() const
            {
                return (m_primitive_stream);
            }

            graphics::primitive_stream_descriptor::ref get_primitive_stream()
            {
                return (m_primitive_stream);
            }

            void set_primitive_stream(graphics::primitive_stream_descriptor::ref const& primitive_stream)
            {
                m_primitive_stream = primitive_stream;
            }

            int get_element_count() const
            {
                return (m_element_count);
            }

            void set_element_count(int element_count)
            {
                m_element_count = element_count;
            }

            int get_index_buffer_start_offset() const
            {
                return (m_index_buffer_start_offset);
            }

            void set_index_buffer_start_offset(int index_buffer_start_offset)
            {
                m_index_buffer_start_offset = index_buffer_start_offset;
            }

            int get_index_value_offset() const
            {
                return (m_index_value_offset);
            }

            void set_index_value_offset(int index_value_offset)
            {
                m_index_value_offset = index_value_offset;
            }

            math::f32aabb const* get_aabb() const
            {
                return (&m_aabb);
            }

            void set_aabb(math::f32aabb const* aabb)
            {
                m_aabb = *aabb;
            }

            void compute_aabb();

        private:
            geometry_descriptor()
                : m_element_count(0)
                , m_index_buffer_start_offset(0)
                , m_index_value_offset(0)
            {}

            graphics::primitive_stream_descriptor::ref m_primitive_stream;

            // Select a subset of the stream
            int m_element_count;
            int m_index_buffer_start_offset; // Offset into the IBO
            int m_index_value_offset; // Value to add to each index read from the IBO
            math::f32aabb m_aabb;

            // Disallowed operations:
            explicit geometry_descriptor(geometry_descriptor const&);
            geometry_descriptor& operator =(geometry_descriptor const&);
        };
    }
}
