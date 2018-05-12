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
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/graphics/texture_interface.hpp"

namespace electroslag {
    namespace renderer {
        class texture_manager {
        public:
            texture_manager();
            ~texture_manager();

            void initialize();
            void shutdown();

            graphics::texture_interface::ref const& get_texture(
                unsigned long long hash
                ) const;

            graphics::texture_interface::ref const& get_texture(
                graphics::texture_descriptor::ref& desc
                );

        private:
            mutable threading::mutex m_mutex;

            typedef std::unordered_map<
                unsigned long long,
                graphics::texture_interface::ref,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > texture_table;
            texture_table m_texture_table;

            // Disallowed operations:
            explicit texture_manager(texture_manager const&);
            texture_manager& operator =(texture_manager const&);
        };
    }
}
