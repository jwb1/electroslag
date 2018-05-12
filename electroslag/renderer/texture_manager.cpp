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
#include "electroslag/renderer/texture_manager.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    namespace renderer {
        texture_manager::texture_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:texture_manager"))
        {}

        texture_manager::~texture_manager()
        {
            shutdown();
        }

        void texture_manager::initialize()
        {
        }

        void texture_manager::shutdown()
        {
            threading::lock_guard texture_manager_lock(&m_mutex);
            m_texture_table.clear();
        }

        graphics::texture_interface::ref const& texture_manager::get_texture(
            unsigned long long hash
            ) const
        {
            threading::lock_guard texture_manager_lock(&m_mutex);
            texture_table::const_iterator t(m_texture_table.find(hash));
            if (t != m_texture_table.end()) {
                return ((*t).second);
            }
            else {
                return (graphics::texture_interface::ref::null_ref);
            }
        }

        graphics::texture_interface::ref const& texture_manager::get_texture(
            graphics::texture_descriptor::ref& desc
            )
        {
            threading::lock_guard texture_manager_lock(&m_mutex);
            texture_table::const_iterator t(m_texture_table.find(desc->get_hash()));
            if (t == m_texture_table.end()) {
                t = m_texture_table.insert(
                    std::make_pair(
                        desc->get_hash(),
                        graphics::get_graphics()->create_texture(desc)
                        )
                    ).first;
            }

            return ((*t).second);
        }
    }
}
