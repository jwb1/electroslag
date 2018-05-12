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
#include "electroslag/threading/thread_pool.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/static_mesh.hpp"
#include "electroslag/renderer/renderer.hpp"
#include "electroslag/renderer/pass_interface.hpp"

namespace electroslag {
    namespace renderer {
        // static
        bool static_mesh::supported_component_bits(int component_bits)
        {
            // A static mesh must have geometry. Default transform and pipelines can be provided.
            switch (component_bits) {
            case renderable_descriptor_component_bits_geometry:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_transform:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_geometry_shape_only:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_geometry_shape_only | renderable_descriptor_component_bits_transform:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_pipeline:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_transform | renderable_descriptor_component_bits_pipeline:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_geometry_shape_only | renderable_descriptor_component_bits_pipeline:
            case renderable_descriptor_component_bits_geometry | renderable_descriptor_component_bits_geometry_shape_only | renderable_descriptor_component_bits_transform | renderable_descriptor_component_bits_pipeline:
                return (true);

            default:
                return (false);
            }
        }

        static_mesh::static_mesh(
            renderable_descriptor::ref const& desc,
            transform_descriptor::ref const& transform,
            unsigned long long name_hash
            )
            : mesh_interface(name_hash)
            , m_element_count(0)
            , m_index_buffer_start_offset(0)
            , m_index_value_offset(0)
            , m_dynamic_ubo_size(0)
            , m_dynamic_ubo_offset(0)
            , m_initialization_step(initialization_step_wait_for_pipelines)
            , m_local_to_world_dirty(false)
        {
            renderer* r = get_renderer_internal();

            // Handle the geometry data.
            ELECTROSLAG_CHECK(desc->get_component_bits() & renderable_descriptor_component_bits_geometry);
            m_element_count = desc->get_geometry_component()->get_element_count();
            m_index_buffer_start_offset = desc->get_geometry_component()->get_index_buffer_start_offset();
            m_index_value_offset = desc->get_geometry_component()->get_index_value_offset();
            m_local_aabb = *(desc->get_geometry_component()->get_aabb());

            m_primitive_stream = r->get_primitive_stream_manager()->get_stream(
                desc->get_geometry_component()->get_primitive_stream()
                );

            // Load transform data in to properties.
            bool transform_component = (desc->get_component_bits() & renderable_descriptor_component_bits_transform) != 0;
            bool transform_instance = transform.is_valid();
            if (transform_component && transform_instance) {
                // Quaternion rotation is not commutative. (Component rotation before instance.)
                m_rotation.reset_value(transform->get_rotation() * desc->get_transform_component()->get_rotation());
                m_scale.reset_value(transform->get_scale() * desc->get_transform_component()->get_scale());
                m_translate.reset_value(transform->get_translate() * desc->get_transform_component()->get_translate());
            }
            else if (transform_instance) {
                m_rotation.reset_value(transform->get_rotation());
                m_scale.reset_value(transform->get_scale());
                m_translate.reset_value(transform->get_translate());
            }
            else if (transform_component) {
                m_rotation.reset_value(desc->get_transform_component()->get_rotation());
                m_scale.reset_value(desc->get_transform_component()->get_scale());
                m_translate.reset_value(desc->get_transform_component()->get_translate());
            }
            else {
                m_rotation.reset_value(glm::f32quat(1.0f, 0.0f, 0.0f, 0.0f));
                m_scale.reset_value(glm::f32vec3(1.0f, 1.0f, 1.0f));
                m_translate.reset_value(glm::f32vec3(1.0f, 1.0f, 1.0f));
            }

            // Load the pipelines we need for current passes that will need to render this
            // mesh (currently, the geometry passes.)
            if (!(desc->get_component_bits() & renderable_descriptor_component_bits_pipeline)) {
                // TODO: Allow there to be a default pipeline to insert 4if component bit is not set
                throw std::runtime_error("missing pipeline descriptor bit");
            }

            m_per_pass.set_entries(r->get_geometry_pass_count());

            renderer::pass_iterator p(r->begin_passes());
            renderer::pass_iterator p_end(r->end_passes());
            int per_pass_index = 0;
            while (p != p_end) {
                pass_interface::ref pass(*p);
                if (pass->get_pass_type() == pass_type_geometry) {
                    pipeline_type type = (*p)->get_pipeline_type();

                    m_per_pass.emplace(
                        per_pass_index,
                        pass,
                        desc->get_pipeline_component(type),
                        desc->get_geometry_component()->get_primitive_stream()->get_fields()
                        );
                }
                ++p;
            }
        }

        bool static_mesh::locate_field_source(graphics::shader_field const* field, void const** field_source) const
        {
            if (field->get_kind() == graphics::field_kind_uniform_local_to_world) {
                *field_source = &m_local_to_world;
                return (true);
            }
            else {
                return (false);
            }
        }

        void static_mesh::set_controller(unsigned long long name_hash, animation::property_controller_interface::ref& controller)
        {
            if (name_hash == m_rotation.get_name_hash()) {
                m_rotation.set_controller(controller);
            }
            else if (name_hash == m_scale.get_name_hash()) {
                m_scale.set_controller(controller);
            }
            else if (name_hash == m_translate.get_name_hash()) {
                m_translate.set_controller(controller);
            }
            else {
                throw std::runtime_error("Invalid property name hash");
            }
        }

        void static_mesh::clear_controller(unsigned long long name_hash)
        {
            if (name_hash == m_rotation.get_name_hash()) {
                m_rotation.clear_controller();
            }
            else if (name_hash == m_scale.get_name_hash()) {
                m_scale.clear_controller();
            }
            else if (name_hash == m_translate.get_name_hash()) {
                m_translate.clear_controller();
            }
            else {
                throw std::runtime_error("Invalid property name hash");
            }
        }

        mesh_interface::mesh_transform_work_item::ref static_mesh::make_transform_work_item(
            frame_details* this_frame_details
            )
        {
            return (threading::get_frame_thread_pool()->enqueue_work_item<static_mesh_transform_work_item>(
                ref(this),
                this_frame_details
                ).cast<mesh_interface::mesh_transform_work_item>());
        }

        mesh_interface::mesh_render_work_item::ref static_mesh::make_render_work_item(
            frame_work_item::ref const& mesh_transform,
            frame_details* this_frame_details
            )
        {
            return (threading::get_frame_thread_pool()->enqueue_work_item<static_mesh_render_work_item>(
                mesh_transform,
                ref(this),
                this_frame_details
                ).cast<mesh_interface::mesh_render_work_item>());
        }

        void static_mesh::write_dynamic_ubo(
            pipeline_type type,
            frame_details* this_frame_details
            ) const
        {
            m_per_pass[type].write_dynamic_ubo(this_frame_details);
        }

        void static_mesh::draw(
            graphics::context_interface* context,
            pipeline_type type,
            frame_details* this_frame_details
            )
        {
            m_per_pass[type].bind(context, this_frame_details);
            context->bind_primitive_stream(m_primitive_stream);
            context->draw(m_element_count, m_index_buffer_start_offset, m_index_value_offset);
        }

        bool static_mesh::is_semi_transparent(pipeline_type type) const
        {
            return (m_per_pass[type].get_pipeline()->is_transparent());
        }

        bool static_mesh::is_skybox() const
        {
            return (false);
        }

        void static_mesh::initialize_step(frame_details* this_frame_details)
        {
            renderer* r = this_frame_details->r;

            switch (m_initialization_step) {
            case initialization_step_wait_for_pipelines: {
                // Wait for all of the pipelines to report they are ready; sum up dynamic ubo usage.
                m_dynamic_ubo_size = 0;

                per_pass_vector::const_iterator p(m_per_pass.begin());
                bool pipelines_ready = true;
                while (p != m_per_pass.end()) {
                    pipeline_interface::ref const& pipeline = p->get_pipeline();
                    if (pipeline->ready()) {
                        m_dynamic_ubo_size += pipeline->get_dynamic_ubo_size();
                    }
                    else {
                        pipelines_ready = false;
                        break;
                    }
                    ++p;
                }

                if (pipelines_ready) {
                    m_initialization_step = initialization_step_request_ubo;
                }
                break;
            }

            case initialization_step_request_ubo: {
                // Assign a offset in the dynamic ubo to each pass's pipeline.
                int base_dynamic_ubo_offset = r->get_ubo_manager()->request_dynamic_ubo_space(m_dynamic_ubo_size);
                int current_ubo_offset = 0;

                // This list of places to look for dynamic UBO source pointers:
                // [renderer, static_mesh_renderable, pass, mesh_data_per_pass, pipeline]
                field_source_list field_source;
                field_source.emplace_back(r);
                field_source.emplace_back(this);

                per_pass_vector::iterator p(m_per_pass.begin());
                while (p != m_per_pass.end()) {
                    mesh_data_per_pass* md = &(*p);

                    // Assign an offset to the pass within the whole mesh's dynamic UBO region.
                    md->set_dynamic_ubo_offset(base_dynamic_ubo_offset + current_ubo_offset);
                    current_ubo_offset += md->get_pipeline()->get_dynamic_ubo_size();

                    // Create the passes' dynamic ubo write acceleration table
                    field_source.emplace_back(md->get_pass().get_pointer());
                    field_source.emplace_back(md);
                    field_source.emplace_back(md->get_pipeline().get_pointer());

                    md->create_dynamic_ubo_writes(field_source);

                    field_source.pop_back();
                    field_source.pop_back();
                    field_source.pop_back();

                    ++p;
                }

                m_initialization_step = initialization_step_wait_for_ubo;
                break;
            }

            case initialization_step_wait_for_ubo: {
                if (m_dynamic_ubo_size + m_dynamic_ubo_offset <= this_frame_details->dynamic_ubo_size) {
                    m_initialization_step = initialization_step_ready;
                    m_local_to_world_dirty = true;
                }
                break;
            }
            }
        }

        void static_mesh::transform(frame_details* this_frame_details)
        {
            if (m_initialization_step != initialization_step_ready) {
                initialize_step(this_frame_details);
            }

            if (m_initialization_step == initialization_step_ready) {
                // Animate properties
                m_local_to_world_dirty |= (m_translate.update(this_frame_details->millisec_elapsed) == animation::property_value_state_changed);
                m_local_to_world_dirty |= (m_scale.update(this_frame_details->millisec_elapsed) == animation::property_value_state_changed);
                m_local_to_world_dirty |= (m_rotation.update(this_frame_details->millisec_elapsed) == animation::property_value_state_changed);

                // Update transformation matrices and world space coordinates if necessary.
                if (m_local_to_world_dirty) {
                    // Need to translate the mesh to be centered for correct scaling / rotation.
                    // We could precompute this and keep a member variable. Not sure that is faster.
                    glm::f32vec3 center_translate(
                        ((m_local_aabb.get_max_corner() - m_local_aabb.get_min_corner()) / 2.0f) + m_local_aabb.get_min_corner()
                        );

                    // M = T * S * R * CT
                    // Geometrically: center translate, rotate, then scale, then translate
                    m_local_to_world = glm::translate(m_translate.get_value() + center_translate);
                    m_local_to_world = glm::scale(m_local_to_world, m_scale.get_value());
                    m_local_to_world = m_local_to_world * glm::mat4_cast(m_rotation.get_value());
                    m_local_to_world = m_local_to_world * glm::translate(-center_translate);

                    m_world_aabb = m_local_aabb * m_local_to_world;

                    m_local_to_world_dirty = false;
                }
            }
        }

        void static_mesh::render(
            frame_details* this_frame_details
            )
        {
            if (m_initialization_step == initialization_step_ready) {
                mesh_interface::ref this_ref(this);
                per_pass_vector::iterator p(m_per_pass.begin());
                while (p != m_per_pass.end()) {
                    p->get_pass()->render_mesh_in_pass(this_ref, this_frame_details);
                    ++p;
                }
            }
        }
    }
}
