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
#include "electroslag/renderer/pipeline_manager.hpp"
#include "electroslag/renderer/pipeline_composite.hpp"

namespace electroslag {
    namespace renderer {
        pipeline_manager::pipeline_manager()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:pipeline_manager"))
        {}

        pipeline_manager::~pipeline_manager()
        {
            shutdown();
        }

        void pipeline_manager::initialize()
        {
        }

        void pipeline_manager::shutdown()
        {
            threading::lock_guard pipeline_manager_lock(&m_mutex);
            m_pipeline_table.clear();
        }

        pipeline_interface::ref const& pipeline_manager::get_pipeline(
            unsigned long long hash
            ) const
        {
            threading::lock_guard pipeline_manager_lock(&m_mutex);
            pipeline_table::const_iterator p(m_pipeline_table.find(hash));
            if (p != m_pipeline_table.end()) {
                return ((*p).second);
            }
            else {
                return (pipeline_interface::ref::null_ref);
            }
        }

        pipeline_interface::ref const& pipeline_manager::get_pipeline(
            pipeline_descriptor::ref const& desc,
            graphics::shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            threading::lock_guard pipeline_manager_lock(&m_mutex);

            // TODO: Hash XOR as new hash?!
            unsigned long long pipeline_instance_hash = desc->get_hash() ^ vertex_attrib_field_map->get_hash();

            pipeline_table::const_iterator p(m_pipeline_table.find(pipeline_instance_hash));
            if (p == m_pipeline_table.end()) {
                pipeline_interface::ref new_pipeline(pipeline_composite::create(desc, vertex_attrib_field_map).cast<pipeline_interface>());
                if (!new_pipeline->ready()) {
                    m_not_ready_pipelines.emplace_back(new_pipeline);
                }
                p = m_pipeline_table.insert(
                    std::make_pair(pipeline_instance_hash, new_pipeline)
                    ).first;
            }

            return ((*p).second);
        }

        void pipeline_manager::prepare_pipelines_for_frame(frame_details*)
        {
            // This method is called once a frame to pump pipeline initialization stepping.
            // Initialization should only be stepped once a frame, and as pipelines are
            // potentially shared among many meshes, this allows for correct init.

            threading::lock_guard pipeline_manager_lock(&m_mutex);

            // Reverse iteration allows us to erase from the vector sanely.
            int not_ready_pipelines = static_cast<int>(m_not_ready_pipelines.size());
            for (int i = not_ready_pipelines - 1; i >= 0; --i) {
                m_not_ready_pipelines.at(i)->initialize_step();

                if (m_not_ready_pipelines.at(i)->ready()) {
                    m_not_ready_pipelines.erase(m_not_ready_pipelines.begin() + i);
                }
            }
        }
    }
}
