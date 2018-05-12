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
#include "electroslag/graphics/primitive_stream_descriptor.hpp"
#include "electroslag/graphics/primitive_stream_interface.hpp"

namespace electroslag {
    namespace renderer {
        class primitive_stream_manager {
        public:
            primitive_stream_manager();
            ~primitive_stream_manager();

            void initialize();
            void shutdown();

            graphics::primitive_stream_interface::ref const& get_stream(
                unsigned long long hash
                ) const;

            graphics::primitive_stream_interface::ref const& get_stream(
                graphics::primitive_stream_descriptor::ref const& desc
                );

        private:
            mutable threading::mutex m_mutex;

            typedef std::unordered_map<
                unsigned long long,
                graphics::primitive_stream_interface::ref,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > primitive_stream_table;
            primitive_stream_table m_primitive_stream_table;

            // Disallowed operations:
            explicit primitive_stream_manager(primitive_stream_manager const&);
            primitive_stream_manager& operator =(primitive_stream_manager const&);
        };
    }
}
