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

namespace electroslag {
    namespace graphics {
        class shader_field_map
            : public referenced_object
            , public serialize::serializable_object<shader_field_map> {
        public:
            typedef reference<shader_field_map> ref;
        private:
            typedef std::unordered_map<
                unsigned long long,
                shader_field*,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > field_table;
        public:
            typedef field_table::const_iterator const_iterator;
            typedef field_table::iterator iterator;

            static ref create()
            {
                return (ref(new shader_field_map()));
            }

            static ref clone(ref const& copy_ref)
            {
                return (ref(new shader_field_map(*copy_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit shader_field_map(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            void insert(shader_field* field)
            {
                m_map.insert(std::make_pair(field->get_hash(), field));
            }

            int get_field_count() const
            {
                return (static_cast<int>(m_map.size()));
            }

            const_iterator begin() const
            {
                return (m_map.begin());
            }

            iterator begin()
            {
                return (m_map.begin());
            }

            const_iterator end() const
            {
                return (m_map.end());
            }

            iterator end()
            {
                return (m_map.end());
            }

            shader_field const* find(unsigned long long name_hash) const
            {
                return (m_map.at(name_hash));
            }

            shader_field* find(unsigned long long name_hash)
            {
                return (m_map.at(name_hash));
            }

            int get_fields_total_size() const;

        private:
            shader_field_map()
            {}

            explicit shader_field_map(shader_field_map const& copy_object)
                : referenced_object()
                , serializable_object(copy_object)
                , m_map(copy_object.m_map)
            {}

            field_table m_map;

            // Disallowed operations:
            shader_field_map& operator =(shader_field_map const&);
        };
    }
}
