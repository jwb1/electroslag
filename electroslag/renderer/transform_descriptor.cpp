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
#include "electroslag/renderer/transform_descriptor.hpp"

namespace electroslag {
    namespace renderer {
        transform_descriptor::transform_descriptor(serialize::archive_reader_interface* ar)
            : m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
            , m_scale(1.0f, 1.0f, 1.0f)
            , m_translate(1.0f, 1.0f, 1.0f)
        {
            // Rotation fields just overwrite each other.
            if (!ar->read_buffer("rotation", &m_rotation, sizeof(m_rotation))) {
                glm::f32vec3 euler_angles;
                if (ar->read_buffer("rotation_euler_angle_deg", &euler_angles, sizeof(euler_angles))) {
                    m_rotation = glm::f32quat(glm::radians(euler_angles));
                }
                if (ar->read_buffer("rotation_euler_angle", &euler_angles, sizeof(euler_angles))) {
                    m_rotation = glm::f32quat(euler_angles);
                }
            }
            ar->read_buffer("scale", &m_scale, sizeof(m_scale));
            ar->read_buffer("translate", &m_translate, sizeof(m_translate));
        }

        void transform_descriptor::save_to_archive(serialize::archive_writer_interface* ar)
        {
            serializable_object::save_to_archive(ar);

            ar->write_buffer("rotation", &m_rotation, sizeof(m_rotation));
            ar->write_buffer("scale", &m_scale, sizeof(m_scale));
            ar->write_buffer("translate", &m_translate, sizeof(m_translate));
        }
    }
}
