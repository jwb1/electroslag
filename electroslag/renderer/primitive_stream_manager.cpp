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
#include "electroslag/renderer/primitive_stream_manager.hpp"
#include "electroslag/graphics/graphics_interface.hpp"

namespace electroslag {
    namespace renderer {
        primitive_stream_manager::primitive_stream_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:primitive_stream_manager"))
        {}

        primitive_stream_manager::~primitive_stream_manager()
        {
            shutdown();
        }

        void primitive_stream_manager::initialize()
        {
        }

        void primitive_stream_manager::shutdown()
        {
            threading::lock_guard primitive_stream_manager_lock(&m_mutex);
            m_primitive_stream_table.clear();
        }

        graphics::primitive_stream_interface::ref const& primitive_stream_manager::get_stream(
            unsigned long long hash
            ) const
        {
            threading::lock_guard primitive_stream_manager_lock(&m_mutex);
            primitive_stream_table::const_iterator ps(m_primitive_stream_table.find(hash));
            if (ps != m_primitive_stream_table.end()) {
                return ((*ps).second);
            }
            else {
                return (graphics::primitive_stream_interface::ref::null_ref);
            }
        }

        graphics::primitive_stream_interface::ref const& primitive_stream_manager::get_stream(
            graphics::primitive_stream_descriptor::ref const& desc
            )
        {
            threading::lock_guard primitive_stream_manager_lock(&m_mutex);
            primitive_stream_table::const_iterator ps(m_primitive_stream_table.find(desc->get_hash()));
            if (ps == m_primitive_stream_table.end()) {
                ps = m_primitive_stream_table.insert(
                    std::make_pair(
                        desc->get_hash(),
                        graphics::get_graphics()->create_primitive_stream(desc)
                        )
                    ).first;
            }

            return ((*ps).second);
        }
    }
}
