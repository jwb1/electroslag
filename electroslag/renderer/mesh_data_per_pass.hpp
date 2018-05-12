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
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/renderer/field_source_interface.hpp"
#include "electroslag/renderer/pass_interface.hpp"
#include "electroslag/renderer/pipeline_descriptor.hpp"
#include "electroslag/renderer/pipeline_interface.hpp"

namespace electroslag {
    namespace renderer {
        class mesh_data_per_pass : public field_source_interface {
        public:
            mesh_data_per_pass(
                pass_interface::ref const& pass,
                pipeline_descriptor::ref const& pipeline_desc,
                graphics::shader_field_map::ref const& vertex_attrib_field_map
                );

            // Implement field_source_interface
            virtual bool locate_field_source(graphics::shader_field const* field, void const** field_source) const;

            // mesh_data_per_pass methods
            void create_dynamic_ubo_writes(field_source_list const& field_sources);

            pass_interface::ref const& get_pass() const
            {
                return (m_pass);
            }

            pass_interface::ref& get_pass()
            {
                return (m_pass);
            }

            pipeline_interface::ref const& get_pipeline() const
            {
                return (m_pipeline);
            }

            pipeline_interface::ref& get_pipeline()
            {
                return (m_pipeline);
            }

            void set_local_to_clip(glm::f32mat4x4 const& local_to_clip)
            {
                m_local_to_clip = local_to_clip;
            }

            void set_dynamic_ubo_offset(int ubo_offset)
            {
                ELECTROSLAG_CHECK(ubo_offset >= 0);
                m_dynamic_ubo_offset = ubo_offset;
            }

            void write_dynamic_ubo(frame_details* this_frame_details) const
            {
                byte* base_pointer = this_frame_details->mapped_dynamic_ubo + m_dynamic_ubo_offset;

                dynamic_ubo_field_write_vector::const_iterator d(m_dynamic_ubo_writes.begin());
                while (d != m_dynamic_ubo_writes.end()) {
                    memcpy(base_pointer + d->field_offset, d->field_source, d->size);
                    ++d;
                }
            }

            void bind(graphics::context_interface* context, frame_details* this_frame_details)
            {
                ELECTROSLAG_CHECK(m_dynamic_ubo_offset >= 0);
                m_pipeline->bind_pipeline(
                    context,
                    this_frame_details,
                    m_dynamic_ubo_offset
                    );
            }

        private:
            int create_dynamic_ubo_writes_per_stage(
                graphics::shader_stage_descriptor::ref const& stage_desc,
                field_source_list const& field_sources,
                int current_dynamic_ubo_offset
                );

            void create_dynamic_ubo_writes_per_ubo(
                graphics::uniform_buffer_descriptor::ref const& ubo_desc,
                field_source_list const& field_sources,
                int current_dynamic_ubo_offset
                );

            pass_interface::ref m_pass;
            glm::f32mat4x4 m_local_to_clip;
            pipeline_interface::ref m_pipeline;
            int m_dynamic_ubo_offset;

            // Dynamic UBOs are populated in an optimized loop; reaching out all
            // over the place!
            struct dynamic_ubo_field_write {
                // Required operator for sorting the writes; sort by destination offset.
                bool operator <(dynamic_ubo_field_write const& compare_with) const
                {
                    return (field_offset < compare_with.field_offset);
                }

                int field_offset;
                void const* field_source;
                int size;
            };
            typedef std::vector<dynamic_ubo_field_write> dynamic_ubo_field_write_vector;
            dynamic_ubo_field_write_vector m_dynamic_ubo_writes;

            // Disallowed operations:
            mesh_data_per_pass();
            explicit mesh_data_per_pass(mesh_data_per_pass const&);
            mesh_data_per_pass& operator =(mesh_data_per_pass const&);
        };
    }
}
