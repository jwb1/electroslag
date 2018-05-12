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
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/uniform_buffer_manager.hpp"
#include "electroslag/renderer/renderer.hpp"

namespace electroslag {
    namespace renderer {
        uniform_buffer_manager::uniform_buffer_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:uniform_buffer_manager"))
            , m_current_frame_index(0)
            , m_allocated_ubo_size(0)
        {}

        uniform_buffer_manager::~uniform_buffer_manager()
        {
            shutdown();
        }

        void uniform_buffer_manager::initialize()
        {
        }

        void uniform_buffer_manager::shutdown()
        {
            threading::lock_guard uniform_buffer_manager_lock(&m_mutex);
            m_buffer_table.clear();

            for (int i = 0; i < per_frame_count; ++i) {
                m_per_frame_buffers[i].buffer.reset();
                m_per_frame_buffers[i].current_size = 0;
            }
        }

        graphics::buffer_interface::ref const& uniform_buffer_manager::get_buffer(
            unsigned long long ubo_hash
            ) const
        {
            threading::lock_guard uniform_buffer_manager_lock(&m_mutex);
            buffer_table::const_iterator b(m_buffer_table.find(ubo_hash));
            if (b != m_buffer_table.end()) {
                return ((*b).second);
            }
            else {
                return (graphics::buffer_interface::ref::null_ref);
            }
        }

        graphics::buffer_interface::ref const& uniform_buffer_manager::get_buffer(
            graphics::buffer_descriptor::ref const& desc
            )
        {
            threading::lock_guard uniform_buffer_manager_lock(&m_mutex);
            buffer_table::const_iterator b(m_buffer_table.find(desc->get_hash()));
            if (b == m_buffer_table.end()) {
                b = m_buffer_table.insert(
                    std::make_pair(
                        desc->get_hash(),
                        graphics::get_graphics()->create_buffer(desc)
                        )
                    ).first;
            }

            return ((*b).second);
        }

        int uniform_buffer_manager::request_dynamic_ubo_space(int dynamic_ubo_size)
        {
            threading::lock_guard uniform_buffer_manager_lock(&m_mutex);
            int allocation_offset = m_allocated_ubo_size;
            m_allocated_ubo_size += dynamic_ubo_size;
            return (allocation_offset);
        }

        void uniform_buffer_manager::prepare_dynamic_ubo_for_frame(frame_details* this_frame_details)
        {
            threading::lock_guard uniform_buffer_manager_lock(&m_mutex);

            // Rotate through the per-frame data structures.
            m_current_frame_index++;
            if (m_current_frame_index == per_frame_count) {
                m_current_frame_index = 0;
            }
            per_frame_dynamic_ubo* this_frame_ubo = &m_per_frame_buffers[m_current_frame_index];

            // See if there is a completed buffer resize for this frame.
            if (this_frame_ubo->pending_buffer.is_valid() &&
                this_frame_ubo->pending_buffer->is_finished()) {

                // Move the pending buffer over to become the actual buffer.
                this_frame_ubo->buffer = this_frame_ubo->pending_buffer;
                this_frame_ubo->current_size = this_frame_ubo->pending_size;
                this_frame_ubo->mapped_pointer = this_frame_ubo->buffer->map();

                this_frame_ubo->pending_buffer.reset();
                this_frame_ubo->pending_size = 0;
            }

            // This frame might need more UBO space than the previous, so create a resize request.
            // Allow pending resize requests to finish before starting a new one.
            if (this_frame_ubo->current_size < m_allocated_ubo_size && !this_frame_ubo->pending_buffer.is_valid()) {
                graphics::buffer_descriptor::ref buffer_desc(graphics::buffer_descriptor::create());
                buffer_desc->set_buffer_memory_caching(graphics::buffer_memory_caching_coherent);
                buffer_desc->set_buffer_memory_map(graphics::buffer_memory_map_write);
                buffer_desc->set_uninitialized_data_size(m_allocated_ubo_size);

                this_frame_ubo->pending_buffer = graphics::get_graphics()->create_buffer(buffer_desc);
                this_frame_ubo->pending_size = m_allocated_ubo_size;
            }

            // Pass the dynamic UBO details to the frame.
            this_frame_details->dynamic_ubo_size = this_frame_ubo->current_size;
            this_frame_details->dynamic_ubo = this_frame_ubo->buffer;
            this_frame_details->mapped_dynamic_ubo = this_frame_ubo->mapped_pointer;
        }
    }
}
