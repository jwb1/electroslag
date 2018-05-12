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

namespace electroslag {
    namespace renderer {
        class transform_descriptor
            : public referenced_object
            , public serialize::serializable_object<transform_descriptor> {
        public:
            typedef reference<transform_descriptor> ref;

            static ref create()
            {
                return (ref(new transform_descriptor()));
            }

            // Implement serializable_object
            explicit transform_descriptor(serialize::archive_reader_interface* ar);
            virtual void save_to_archive(serialize::archive_writer_interface* ar);

            glm::f32quat get_rotation() const
            {
                return (m_rotation);
            }

            void set_rotation(glm::f32quat rotation)
            {
                m_rotation = rotation;
            }

            glm::f32vec3 get_scale() const
            {
                return (m_scale);
            }

            void set_scale(glm::f32vec3 scale)
            {
                m_scale = scale;
            }

            glm::f32vec3 get_translate() const
            {
                return (m_translate);
            }

            void set_translate(glm::f32vec3 translate)
            {
                m_translate = translate;
            }

        private:
            transform_descriptor()
                : m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
                , m_scale(1.0f, 1.0f, 1.0f)
                , m_translate(1.0f, 1.0f, 1.0f)
            {}

            glm::f32quat m_rotation;
            glm::f32vec3 m_scale;
            glm::f32vec3 m_translate;

            // Disallowed operations:
            explicit transform_descriptor(transform_descriptor const&);
            transform_descriptor& operator =(transform_descriptor const&);
        };
    }
}
