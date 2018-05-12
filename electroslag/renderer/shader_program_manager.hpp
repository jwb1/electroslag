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
#include "electroslag/graphics/shader_program_descriptor.hpp"
#include "electroslag/graphics/shader_program_interface.hpp"

namespace electroslag {
    namespace renderer {
        class shader_program_manager {
        public:
            shader_program_manager();
            ~shader_program_manager();

            void initialize();
            void shutdown();

            graphics::shader_program_interface::ref const& get_shader_program(
                unsigned long long hash
                ) const;

            graphics::shader_program_interface::ref const& get_shader_program(
                graphics::shader_program_descriptor::ref const& desc,
                graphics::shader_field_map::ref const& vertex_attrib_field_map
                );

        private:
            mutable threading::mutex m_mutex;

            typedef std::unordered_map<
                unsigned long long,
                graphics::shader_program_interface::ref,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > shader_program_table;
            shader_program_table m_shader_program_table;

            // Disallowed operations:
            explicit shader_program_manager(shader_program_manager const&);
            shader_program_manager& operator =(shader_program_manager const&);
        };
    }
}
