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
#include "electroslag/logger.hpp"
#include "electroslag/renderer/shader_program_manager.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    namespace renderer {
        shader_program_manager::shader_program_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:shader_program_manager"))
        {}

        shader_program_manager::~shader_program_manager()
        {
            shutdown();
        }

        void shader_program_manager::initialize()
        {
        }

        void shader_program_manager::shutdown()
        {
            threading::lock_guard shader_program_manager_lock(&m_mutex);
            m_shader_program_table.clear();
        }

        graphics::shader_program_interface::ref const& shader_program_manager::get_shader_program(
            unsigned long long hash
            ) const
        {
            threading::lock_guard shader_program_manager_lock(&m_mutex);
            shader_program_table::const_iterator s(m_shader_program_table.find(hash));
            if (s != m_shader_program_table.end()) {
                return ((*s).second);
            }
            else {
                return (graphics::shader_program_interface::ref::null_ref);
            }
        }

        graphics::shader_program_interface::ref const& shader_program_manager::get_shader_program(
            graphics::shader_program_descriptor::ref const& desc,
            graphics::shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            threading::lock_guard shader_program_manager_lock(&m_mutex);

            // TODO: Hash XOR as new hash?!
            unsigned long long shader_instance_hash = desc->get_hash() ^ vertex_attrib_field_map->get_hash();

            shader_program_table::const_iterator s(m_shader_program_table.find(shader_instance_hash));
            if (s == m_shader_program_table.end()) {
                s = m_shader_program_table.insert(
                    std::make_pair(
                        shader_instance_hash,
                        graphics::get_graphics()->create_shader_program(desc, vertex_attrib_field_map)
                        )
                    ).first;
            }

            return ((*s).second);
        }
    }
}
