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
#include "electroslag/logger.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/graphics/graphics_types.hpp"

namespace electroslag {
    namespace graphics {
        class shader_field : public serialize::serializable_object<shader_field> {
        public:
            shader_field()
            {
                set_data(field_type_unknown, field_kind_unknown, false, 0, 0);
            }

            shader_field(
                field_type type,
                field_kind kind,
                int index,
                int offset,
                std::string const& name
                )
                : serializable_object(name)
            {
                set_data(type, kind, false, index, offset);
            }

            virtual ~shader_field()
            {}

            // Copy construction and assignment; Copies will be considered "clones" and not
            // serializable because clones have the same name as the original object.
            shader_field(shader_field const& copy_object)
                : serializable_object(copy_object)
                , m_fields(copy_object.m_fields)
            {}

            shader_field& operator =(shader_field const& copy_object)
            {
                if (&copy_object != this) {
                    serializable_object::operator =(copy_object);
                    m_fields = copy_object.m_fields;
                }
                return (*this);
            }

            bool operator ==(shader_field const& compare_with) const
            {
                return ((m_fields.value == compare_with.m_fields.value) &&
                        (get_hash() == compare_with.get_hash()));
            }

            // Implement serializable_object
            explicit shader_field(serialize::archive_reader_interface* ar)
            {
                if (!ar->read_uint64("packed_fields", &m_fields.value)) {
                    field_type type = field_type_unknown;
                    if (!ar->read_enumeration("field_type", &type, field_type_strings)) {
                        throw load_object_failure("field_type");
                    }
                    m_fields.f.type = type;

                    field_kind kind;
                    if (!ar->read_enumeration("field_kind", &kind, field_kind_strings)) {
                        throw load_object_failure("field_kind");
                    }
                    m_fields.f.kind = kind;

                    int32_t active = 1; // Default value;
                    ar->read_int32("active", &active);
                    m_fields.f.active = (active != 0) ? 1 : 0;

                    uint32_t index = 0; // Default value
                    if (ar->read_uint32("index", &index) && index >= (1 << 15)) {
                        throw load_object_failure("index");
                    }
                    m_fields.f.index = index;

                    uint32_t offset = 0; // Default value
                    ar->read_uint32("offset", &offset);
                    m_fields.f.offset = offset;
                }
            }

            void save_to_archive(serialize::archive_writer_interface* ar)
            {
                serializable_object::save_to_archive(ar);
                ar->write_uint64("packed_fields", m_fields.value);
            }

            field_kind get_kind() const
            {
                return (m_fields.f.kind);
            }

            bool is_attribute() const
            {
                return (m_fields.f.kind < field_kind_uniform && m_fields.f.kind != field_kind_unknown);
            }

            bool is_uniform() const
            {
                return (m_fields.f.kind >= field_kind_uniform &&
                        m_fields.f.kind < field_kind_count &&
                        m_fields.f.kind != field_kind_unknown);
            }

            field_type get_field_type() const
            {
                return (m_fields.f.type);
            }

            bool is_active() const
            {
                return (m_fields.f.active);
            }

            void set_active(bool active)
            {
                m_fields.f.active = active;
            }

            int get_index() const
            {
                return (m_fields.f.index);
            }

            void set_index(int index)
            {
                ELECTROSLAG_CHECK(index >= 0 && index < (1 << 15));
                m_fields.f.index = index;
            }

            int get_offset() const
            {
                return (m_fields.f.offset);
            }

            void set_offset(int offset)
            {
                ELECTROSLAG_CHECK(offset >= 0);
                m_fields.f.offset = offset;
            }

            template<class T>
            void write_uniform(byte* mapped_buffer, T const* value) const
            {
#if defined(ELECTROSLAG_BUILD_DEBUG)
                ELECTROSLAG_CHECK(is_uniform());
                ELECTROSLAG_CHECK(get_field_type() == T::get_field_type());
                ELECTROSLAG_CHECK(mapped_buffer != 0 && value != 0);
                if (!is_active()) {
                    ELECTROSLAG_LOG_WARN("Writing to non-active uniform");
                }
#endif
                *reinterpret_cast<T*>(mapped_buffer + get_offset()) = *value;
            }

        private:
            void set_data(field_type type, field_kind kind, bool active, int index, int offset)
            {
                m_fields.f.type = type;
                m_fields.f.kind = kind;
                m_fields.f.active = active;
                m_fields.f.index = index;
                m_fields.f.offset = offset;
            }

            // Encode the type, kind, index, and offset in a single 64 bit value.
            union packed_fields {
                uint64_t value;
                struct each_field {
                    field_type type:8;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(field_type, 8);
                    field_kind kind:8;
                    ELECTROSLAG_ENUM_BITPACK_CHECK(field_kind, 8);
                    unsigned int active:1;
                    unsigned int index:15;
                    unsigned int offset:32;
                } f;
            } m_fields;
            ELECTROSLAG_STATIC_CHECK(sizeof(packed_fields) == sizeof(uint64_t), "Bit packing check");
        };
    }
}
