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
#include "electroslag/serialize/serializable_object.hpp"
#include "electroslag/serialize/archive_interface.hpp"
#include "electroslag/renderer/transform_descriptor.hpp"
#include "electroslag/renderer/renderable_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        class instance_descriptor
            : public referenced_object
            , public serialize::serializable_object<instance_descriptor> {
        public:
            typedef reference<instance_descriptor> ref;
        private:
            typedef std::vector<instance_descriptor::ref> instance_vector;
        public:
            typedef instance_vector::const_iterator const_instance_iterator;
            typedef instance_vector::iterator instance_iterator;

            static ref create()
            {
                return (ref(new instance_descriptor()));
            }

            // Implement serializable_object
            explicit instance_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            // instance_descriptor methods
            void insert_child_instance(instance_descriptor::ref const& descriptor)
            {
                m_child_instances.emplace_back(descriptor);
            }

            const_instance_iterator begin_child_instances() const
            {
                return (m_child_instances.begin());
            }

            instance_iterator begin_child_instances()
            {
                return (m_child_instances.begin());
            }

            const_instance_iterator end_child_instances() const
            {
                return (m_child_instances.end());
            }

            instance_iterator end_child_instances()
            {
                return (m_child_instances.end());
            }

            int get_child_instance_count() const
            {
                return (static_cast<int>(m_child_instances.size()));
            }

            transform_descriptor::ref const& get_transform() const
            {
                return (m_instance_transform);
            }

            transform_descriptor::ref get_transform()
            {
                return (m_instance_transform);
            }

            void set_transform(transform_descriptor::ref const& t)
            {
                m_instance_transform = t;
            }

        private:
            renderable_descriptor::ref m_instance_renderable;
            transform_descriptor::ref m_instance_transform;

            instance_vector m_child_instances;

            // Disallowed operations:
            instance_descriptor();
            explicit instance_descriptor(instance_descriptor const&);
            instance_descriptor& operator =(instance_descriptor const&);
        };
    }
}
