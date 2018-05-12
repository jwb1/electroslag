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
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace graphics {
        class uniform_buffer_descriptor
            : public referenced_object
            , public serialize::serializable_object<uniform_buffer_descriptor> {
        public:
            typedef reference<uniform_buffer_descriptor> ref;

            static ref create()
            {
                return (ref(new uniform_buffer_descriptor()));
            }

            static ref clone(ref const& clone_ref)
            {
                return (ref(new uniform_buffer_descriptor(*clone_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit uniform_buffer_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            int get_binding() const
            {
                return (m_binding);
            }

            void set_binding(int binding)
            {
                ELECTROSLAG_CHECK(binding >= 0);
                m_binding = binding;
            }

            int get_size() const
            {
                return (m_size);
            }

            void set_size(int size)
            {
                ELECTROSLAG_CHECK(size > 0);
                m_size = size;
            }

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

        private:
            uniform_buffer_descriptor()
                : m_binding(-1)
                , m_size(-1)
                , m_fields(shader_field_map::create())
            {}

            explicit uniform_buffer_descriptor(uniform_buffer_descriptor const& copy_object)
                : referenced_object()
                , serializable_object(copy_object)
                , m_binding(copy_object.get_binding())
                , m_size(copy_object.get_size())
                , m_fields(shader_field_map::clone(copy_object.m_fields))
            {}

            int m_binding;
            int m_size;
            shader_field_map::ref m_fields;

            // Disallowed operations:
            uniform_buffer_descriptor& operator =(uniform_buffer_descriptor const&);
        };
    }
}
