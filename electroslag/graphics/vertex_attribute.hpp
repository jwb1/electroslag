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
#include "electroslag/graphics/shader_field.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        class vertex_attribute : public serialize::serializable_object<vertex_attribute> {
        public:
            vertex_attribute()
                : m_stride(0)
                , m_field(0)
            {}

            vertex_attribute(
                shader_field* field,
                buffer_descriptor::ref const& buffer,
                int stride,
                std::string const& name
                )
                : serializable_object(name)
                , m_field(field)
                , m_buffer(buffer)
                , m_stride(stride)
            {
                ELECTROSLAG_CHECK(field->is_attribute());
            }

            vertex_attribute(
                shader_field* field,
                buffer_descriptor::ref const& buffer,
                int stride,
                char const* const name,
                unsigned long long name_hash
                )
                : serializable_object(name, name_hash)
                , m_field(field)
                , m_buffer(buffer)
                , m_stride(stride)
            {
                ELECTROSLAG_CHECK(field->is_attribute());
            }

            virtual ~vertex_attribute()
            {}

            // Copy construction and assignment; Copies will be considered "clones" and not
            // serializable because of name collisions.
            vertex_attribute(vertex_attribute const& copy_object)
                : serializable_object(copy_object)
                , m_field(copy_object.m_field)
                , m_buffer(copy_object.m_buffer)
                , m_stride(copy_object.m_stride)
            {}

            vertex_attribute& operator =(vertex_attribute const& copy_object)
            {
                if (&copy_object != this) {
                    serializable_object::operator =(copy_object);
                    m_field = copy_object.m_field;
                    m_buffer = copy_object.m_buffer;
                    m_stride = copy_object.m_stride;
                }
                return (*this);
            }

            // Implement serializable_object
            explicit vertex_attribute(serialize::archive_reader_interface* ar)
            {
                if (!ar->read_int32("stride", &m_stride)) {
                    throw load_object_failure("stride");
                }

                unsigned long long vbo_name_hash = 0;
                if (!ar->read_name_hash("vbo", &vbo_name_hash)) {
                    throw load_object_failure("vbo");
                }
                m_buffer = serialize::get_database()->find_object_ref<buffer_descriptor>(vbo_name_hash);

                unsigned long long field_name_hash = 0;
                if (!ar->read_name_hash("field", &field_name_hash)) {
                    throw load_object_failure("field");
                }
                m_field = dynamic_cast<shader_field*>(serialize::get_database()->find_object(field_name_hash));

                ELECTROSLAG_CHECK(m_field->is_attribute());
            }

            virtual void save_to_archive(serialize::archive_writer_interface* ar)
            {
                m_buffer->save_to_archive(ar);
                m_field->save_to_archive(ar);

                serializable_object::save_to_archive(ar);
                ar->write_int32("stride", m_stride);
                ar->write_name_hash("vbo", m_buffer->get_hash());
                ar->write_name_hash("field", m_field->get_hash());
            }

            int get_stride() const
            {
                return (m_stride);
            }

            buffer_descriptor::ref const& get_buffer() const
            {
                return (m_buffer);
            }

            shader_field const* get_field() const
            {
                return (m_field);
            }

        private:
            buffer_descriptor::ref m_buffer;
            int m_stride;
            shader_field* m_field;
        };
    }
}
