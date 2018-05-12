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
#include "electroslag/renderer/mesh_data_per_pass.hpp"
#include "electroslag/renderer/renderer.hpp"

namespace electroslag {
    namespace renderer {
        mesh_data_per_pass::mesh_data_per_pass(
            pass_interface::ref const& pass,
            pipeline_descriptor::ref const& pipeline_desc,
            graphics::shader_field_map::ref const& vertex_attrib_field_map
            )
            : m_pass(pass)
            , m_dynamic_ubo_offset(-1)
        {
            m_pipeline = get_renderer_internal()->get_pipeline_manager()->get_pipeline(
                pipeline_desc,
                vertex_attrib_field_map
                );
        }

        bool mesh_data_per_pass::locate_field_source(graphics::shader_field const* field, void const** field_source) const
        {
            if (field->get_kind() == graphics::field_kind_uniform_local_to_clip) {
                *field_source = &m_local_to_clip;
                return (true);
            }
            else {
                return (false);
            }
        }

        void mesh_data_per_pass::create_dynamic_ubo_writes(
            field_source_list const& field_sources
            )
        {
            ELECTROSLAG_CHECK(m_pipeline->ready());
            ELECTROSLAG_CHECK(m_dynamic_ubo_offset >= 0);

            graphics::shader_program_descriptor::ref const& shader = m_pipeline->get_shader()->get_descriptor();
            int current_dynamic_ubo_offset = m_dynamic_ubo_offset;

            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_vertex_shader(), field_sources, current_dynamic_ubo_offset);
            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_tessellation_control_shader(), field_sources, current_dynamic_ubo_offset);
            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_tessellation_evaluation_shader(), field_sources, current_dynamic_ubo_offset);
            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_geometry_shader(), field_sources, current_dynamic_ubo_offset);
            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_fragment_shader(), field_sources, current_dynamic_ubo_offset);
            current_dynamic_ubo_offset = create_dynamic_ubo_writes_per_stage(shader->get_compute_shader(), field_sources, current_dynamic_ubo_offset);

            std::sort(m_dynamic_ubo_writes.begin(), m_dynamic_ubo_writes.end());
            m_dynamic_ubo_writes.shrink_to_fit();
        }

        int mesh_data_per_pass::create_dynamic_ubo_writes_per_stage(
            graphics::shader_stage_descriptor::ref const& stage_desc,
            field_source_list const& field_sources,
            int current_dynamic_ubo_offset
            )
        {
            if (!stage_desc.is_valid()) {
                return (current_dynamic_ubo_offset);
            }

            graphics::shader_stage_descriptor::const_uniform_buffer_iterator u(stage_desc->begin_uniform_buffers());
            while (u != stage_desc->end_uniform_buffers()) {
                create_dynamic_ubo_writes_per_ubo(*u, field_sources, current_dynamic_ubo_offset);
                current_dynamic_ubo_offset += (*u)->get_size();
                ++u;
            }

            return (current_dynamic_ubo_offset);
        }

        void mesh_data_per_pass::create_dynamic_ubo_writes_per_ubo(
            graphics::uniform_buffer_descriptor::ref const& ubo_desc,
            field_source_list const& field_sources,
            int current_dynamic_ubo_offset
            )
        {
            // This would be faster if we could discard UBOs that have all static fields
            // from the iteration.

            graphics::shader_field_map::ref const& field_map = ubo_desc->get_fields();
            graphics::shader_field_map::const_iterator f(field_map->begin());
            while (f != field_map->end()) {
                graphics::shader_field const* field = f->second;

                dynamic_ubo_field_write write;
                write.field_offset = current_dynamic_ubo_offset + field->get_offset();

                // The write source pointer could come from a number of places.
                field_source_list::const_iterator fs(field_sources.begin());
                bool found_field_source = false;
                while (fs != field_sources.end()) {
                    if ((*fs)->locate_field_source(field, &write.field_source)) {
                        found_field_source = true;
                        break;
                    }
                    ++fs;
                }

                if (found_field_source) {
                    write.size = graphics::field_type_util::get_bytes(field->get_field_type());
                    m_dynamic_ubo_writes.emplace_back(write);
                }

                ++f;
            }
        }
    }
}
