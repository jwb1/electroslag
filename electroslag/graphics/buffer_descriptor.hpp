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
#include "electroslag/referenced_buffer.hpp"
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"

namespace electroslag {
    namespace graphics {
        class buffer_descriptor
            : public referenced_object
            , public serialize::serializable_object<buffer_descriptor> {
        public:
            typedef reference<buffer_descriptor> ref;

            static ref create()
            {
                return (ref(new buffer_descriptor()));
            }

            static ref clone(ref const& copy_ref)
            {
                return (ref(new buffer_descriptor(*copy_ref.get_pointer())));
            }

            // Implement serializable_object
            explicit buffer_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            buffer_memory_map get_buffer_memory_map() const
            {
                return (m_memory_map);
            }

            void set_buffer_memory_map(buffer_memory_map memory_map)
            {
                ELECTROSLAG_CHECK(memory_map > buffer_memory_map_unknown &&
                                  memory_map < buffer_memory_map_count);
                m_memory_map = memory_map;
            }

            buffer_memory_caching get_buffer_memory_caching() const
            {
                return (m_memory_caching);
            }

            void set_buffer_memory_caching(buffer_memory_caching memory_caching)
            {
                ELECTROSLAG_CHECK(memory_caching > buffer_memory_caching_unknown &&
                                  memory_caching < buffer_memory_caching_count);
                m_memory_caching = memory_caching;
            }

            bool has_data() const
            {
                return (has_initialized_data() || has_initialized_data());
            }

            void clear_data()
            {
                // Return to a state where "has_data()" would return false.
                m_data.reset();
                m_uninitialized_size = -1;
            }

            bool has_uninitialized_data() const
            {
                return (m_uninitialized_size > 0);
            }

            int get_uninitialized_data_size() const
            {
                if (m_uninitialized_size > 0) {
                    return (m_uninitialized_size);
                }
                else {
                    return (0);
                }
            }

            void set_uninitialized_data_size(int size)
            {
                ELECTROSLAG_CHECK(size > 0);
                m_uninitialized_size = size;
                // Buffer data is either initialized, uninitialized, or cleared.
                m_data.reset();
            }

            bool has_initialized_data() const
            {
                return (m_data.is_valid());
            }

            referenced_buffer_interface::ref const& get_initialized_data() const
            {
                return (m_data);
            }

            referenced_buffer_interface::ref& get_initialized_data()
            {
                return (m_data);
            }

            void set_initialized_data(referenced_buffer_interface::ref const& data)
            {
                m_data = data;
                // Buffer data is either initialized, uninitialized, or cleared
                m_uninitialized_size = -1;
            }

        private:
            buffer_descriptor()
                : m_memory_map(buffer_memory_map_unknown)
                , m_memory_caching(buffer_memory_caching_unknown)
                , m_uninitialized_size(-1)
            {}

            explicit buffer_descriptor(buffer_descriptor const& copy_object)
                : referenced_object()
                , serializable_object(copy_object)
                , m_memory_map(copy_object.m_memory_map)
                , m_memory_caching(copy_object.m_memory_caching)
                , m_uninitialized_size(copy_object.m_uninitialized_size)
                , m_data(cloned_referenced_buffer::create(copy_object.m_data))
            {}

            buffer_memory_map m_memory_map;
            buffer_memory_caching m_memory_caching;
            int m_uninitialized_size;
            referenced_buffer_interface::ref m_data;

            // Disallowed operations:
            buffer_descriptor& operator =(buffer_descriptor const&);
        };
    }
}
