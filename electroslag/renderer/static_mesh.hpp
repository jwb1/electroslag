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
#include "electroslag/dynamic_array.hpp"
#include "electroslag/math/aabb.hpp"
#include "electroslag/graphics/primitive_stream_interface.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/animation/property.hpp"
#include "electroslag/renderer/mesh_interface.hpp"
#include "electroslag/renderer/renderable_descriptor.hpp"
#include "electroslag/renderer/pipeline_interface.hpp"
#include "electroslag/renderer/mesh_data_per_pass.hpp"

namespace electroslag {
    namespace renderer {
        class static_mesh : public mesh_interface {
        public:
            static bool supported_component_bits(int component_bits);

            static mesh_interface::ref create(
                renderable_descriptor::ref const& desc,
                transform_descriptor::ref const& transform,
                unsigned long long name_hash
                )
            {
                return (mesh_interface::ref(new static_mesh(desc, transform, name_hash)));
            }

            virtual ~static_mesh()
            {}

            // Implement field_source_interface
            virtual bool locate_field_source(graphics::shader_field const* field, void const** field_source) const;

            // Implement animated_object
            virtual void set_controller(unsigned long long name_hash, animation::property_controller_interface::ref& controller);
            virtual void clear_controller(unsigned long long name_hash);

            // Implement mesh_interface
            class static_mesh_transform_work_item
                : public mesh_transform_work_item {
            public:
                static_mesh_transform_work_item(
                    mesh_interface::ref& this_mesh,
                    frame_details* this_frame_details
                    )
                    : mesh_transform_work_item(this_mesh, this_frame_details)
                {}

                virtual void execute()
                {
                    m_this_mesh.cast<static_mesh>()->transform(m_this_frame_details);
                }
            };

            virtual mesh_transform_work_item::ref make_transform_work_item(
                frame_details* this_frame_details
                );

            class static_mesh_render_work_item
                : public mesh_render_work_item {
            public:
                static_mesh_render_work_item(
                    frame_work_item::ref const& mesh_transform,
                    mesh_interface::ref& this_mesh,
                    frame_details* this_frame_details
                    )
                    : mesh_render_work_item(mesh_transform, this_mesh, this_frame_details)
                {}

                virtual void execute()
                {
                    m_this_mesh.cast<static_mesh>()->render(m_this_frame_details);
                }
            };

            virtual mesh_render_work_item::ref make_render_work_item(
                frame_work_item::ref const& mesh_transform,
                frame_details* this_frame_details
                );

            virtual void write_dynamic_ubo(
                pipeline_type type,
                frame_details* this_frame_details
                ) const;

            virtual void draw(
                graphics::context_interface* context,
                pipeline_type type,
                frame_details* this_frame_details
                );

            // Implement mesh_interface
            virtual bool is_semi_transparent(pipeline_type type) const;
            virtual bool is_skybox() const;

            virtual math::f32aabb const& get_world_aabb() const
            {
                return (m_world_aabb);
            }

            virtual void compute_local_to_clip(pipeline_type type, glm::f32mat4x4 const& world_to_clip)
            {
                m_per_pass[type].set_local_to_clip(world_to_clip * m_local_to_world);
            }

        protected:
            static_mesh(
                renderable_descriptor::ref const& desc,
                transform_descriptor::ref const& transform,
                unsigned long long name_hash
                );

        private:
            // Static mesh initialization happens over the course of several steps.
            void initialize_step(frame_details* this_frame_details);

            void transform(frame_details* this_frame_details);

            void render(frame_details* this_frame_details);

            // Mesh initialization is spread over steps that occur over multiple frames.
            enum initialization_step {
                initialization_step_unknown = -1,
                initialization_step_wait_for_pipelines = 0,
                initialization_step_request_ubo = 1,
                initialization_step_wait_for_ubo = 2,
                initialization_step_ready = 3
            };
            initialization_step m_initialization_step;

            // Spatial representation; shape defined by primitives, aabb, and transform.
            animation::quat_property<hash_string("rotation")> m_rotation;
            animation::vec3_property<hash_string("scale")> m_scale;
            animation::vec3_property<hash_string("translate")> m_translate;

            glm::f32mat4x4 m_local_to_world;

            math::f32aabb m_local_aabb;
            math::f32aabb m_world_aabb;

            graphics::primitive_stream_interface::ref m_primitive_stream;

            // Draw call parameters.
            int m_element_count;
            int m_index_buffer_start_offset;
            int m_index_value_offset;

            // Total dynamic UBO allocation details.
            int m_dynamic_ubo_size;
            int m_dynamic_ubo_offset;

            // Per pass data.
            typedef dynamic_array<mesh_data_per_pass> per_pass_vector;
            per_pass_vector m_per_pass;

            // Dirty flags.
            bool m_local_to_world_dirty;

            // Disallowed operations:
            static_mesh();
            explicit static_mesh(static_mesh const&);
            static_mesh& operator =(static_mesh const&);
        };
    }
}
