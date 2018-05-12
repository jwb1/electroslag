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
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"
#include "electroslag/renderer/pipeline_interface.hpp"

namespace electroslag {
    namespace renderer {
        class pipeline_manager {
        public:
            pipeline_manager();
            ~pipeline_manager();

            void initialize();
            void shutdown();

            pipeline_interface::ref const& get_pipeline(unsigned long long hash) const;

            pipeline_interface::ref const& get_pipeline(
                pipeline_descriptor::ref const& desc,
                graphics::shader_field_map::ref const& vertex_attrib_field_map
                );

            void prepare_pipelines_for_frame(frame_details*);

        private:
            mutable threading::mutex m_mutex;

            typedef std::vector<pipeline_interface::ref> not_ready_pipeline_vector;
            not_ready_pipeline_vector m_not_ready_pipelines;

            typedef std::unordered_map<
                unsigned long long,
                pipeline_interface::ref,
                prehashed_key<unsigned long long>,
                std::equal_to<unsigned long long>
            > pipeline_table;
            pipeline_table m_pipeline_table;

            // Disallowed operations:
            explicit pipeline_manager(pipeline_manager const&);
            pipeline_manager& operator =(pipeline_manager const&);
        };
    }
}
