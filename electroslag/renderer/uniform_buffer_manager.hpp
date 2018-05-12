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
#include "electroslag/threading/mutex.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/graphics/buffer_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/renderer/scene.hpp"

namespace electroslag {
    namespace renderer {
        class renderer;
        class uniform_buffer_manager {
        public:
            uniform_buffer_manager();
            ~uniform_buffer_manager();

            void initialize();
            void shutdown();

            // Static UBOs are accessed via these methods.
            graphics::buffer_interface::ref const& get_buffer(
                unsigned long long ubo_hash
                ) const;

            graphics::buffer_interface::ref const& get_buffer(
                graphics::buffer_descriptor::ref const& desc
                );

            // Dynamic UBO management via these methods.
            int request_dynamic_ubo_space(int dynamic_ubo_size);

            void prepare_dynamic_ubo_for_frame(frame_details* this_frame_details);

        private:
            mutable threading::mutex m_mutex;

            // Static UBO tracking.
            typedef std::unordered_map<
                unsigned long long,
                graphics::buffer_interface::ref,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > buffer_table;
            buffer_table m_buffer_table;

            // Dynamic UBO tracking.
            static constexpr int const per_frame_count = graphics::context_interface::display_buffer_count + 1;

            struct per_frame_dynamic_ubo {
                per_frame_dynamic_ubo()
                    : current_size(0)
                {}

                graphics::buffer_interface::ref buffer;
                int current_size;
                byte* mapped_pointer;

                graphics::buffer_interface::ref pending_buffer;
                int pending_size;
            } m_per_frame_buffers[per_frame_count];

            int m_current_frame_index;
            int m_allocated_ubo_size;

            // Disallowed operations:
            explicit uniform_buffer_manager(uniform_buffer_manager const&);
            uniform_buffer_manager& operator =(uniform_buffer_manager const&);
        };
    }
}
