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
#include "electroslag/named_object.hpp"
#include "electroslag/math/aabb.hpp"
#include "electroslag/graphics/command_queue_interface.hpp"
#include "electroslag/animation/animated_object.hpp"
#include "electroslag/renderer/renderer_types.hpp"
#include "electroslag/renderer/field_source_interface.hpp"

namespace electroslag {
    namespace renderer {
        class scene;
        class mesh_interface
            : public animation::animated_object
            , public named_object
            , public field_source_interface {
        public:
            typedef reference<mesh_interface> ref;

            virtual ~mesh_interface()
            {}

            // Mesh attributes.
            virtual bool is_semi_transparent(pipeline_type type) const = 0;
            virtual bool is_skybox() const = 0;

            // Meshes are expected to have a world space representation.
            virtual math::f32aabb const& get_world_aabb() const = 0;

            // Compute the local to clip space transformation on behalf of a pass.
            virtual void compute_local_to_clip(pipeline_type type, glm::f32mat4x4 const& world_to_clip) = 0;

            // All mesh items bulk enqueue a work item as part of the transformation
            // process; called by scene.
            class mesh_transform_work_item : public frame_work_item {
            public:
                typedef reference<mesh_transform_work_item> ref;

                mesh_transform_work_item(
                    mesh_interface::ref& this_mesh,
                    frame_details* this_frame_details
                    )
                    : m_this_mesh(this_mesh)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute() = 0;

            protected:
                mesh_interface::ref m_this_mesh;
                frame_details* m_this_frame_details;

            private:
                // Disallowed operations:
                mesh_transform_work_item();
                explicit mesh_transform_work_item(mesh_transform_work_item const&);
                mesh_transform_work_item& operator =(mesh_transform_work_item const&);
            };

            virtual mesh_transform_work_item::ref make_transform_work_item(
                frame_details* this_frame_details
                ) = 0;

            // All mesh objects bulk enqueue a work item as part of the drawing
            // process; called by scene.
            class mesh_render_work_item : public frame_work_item {
            public:
                typedef reference<mesh_render_work_item> ref;

                mesh_render_work_item(
                    frame_work_item::ref const& mesh_transform,
                    mesh_interface::ref& this_mesh,
                    frame_details* this_frame_details
                    )
                    : m_mesh_transform(mesh_transform)
                    , m_this_mesh(this_mesh)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute() = 0;

            protected:
                frame_work_item::ref const m_mesh_transform;
                mesh_interface::ref m_this_mesh;
                frame_details* m_this_frame_details;

            private:
                // Disallowed operations:
                mesh_render_work_item();
                explicit mesh_render_work_item(mesh_render_work_item const&);
                mesh_render_work_item& operator =(mesh_render_work_item const&);
            };

            virtual mesh_render_work_item::ref make_render_work_item(
                frame_work_item::ref const& mesh_transform,
                frame_details* this_frame_details
                ) = 0;

            // The mesh should write out it's dynamic UBO data on behalf of a pass.
            virtual void write_dynamic_ubo(
                pipeline_type type,
                frame_details* this_frame_details
                ) const = 0;

            class mesh_draw_command : public graphics::command {
            public:
                mesh_draw_command(
                    pipeline_type type,
                    mesh_interface::ref& this_mesh,
                    frame_details* this_frame_details
                    )
                    : m_pipeline_type(type)
                    , m_this_mesh(this_mesh)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute(graphics::context_interface* context)
                {
                    m_this_mesh->draw(context, m_pipeline_type, m_this_frame_details);
                }

            private:
                pipeline_type m_pipeline_type;
                mesh_interface::ref m_this_mesh;
                frame_details* m_this_frame_details;
            };

            virtual void draw(
                graphics::context_interface* context,
                pipeline_type type,
                frame_details* this_frame_details
                ) = 0;

        protected:
            mesh_interface()
            {}

            explicit mesh_interface(unsigned long long name_hash)
                : named_object(name_hash)
            {}

        private:
            // Disallowed operations:
            explicit mesh_interface(mesh_interface const&);
            mesh_interface& operator =(mesh_interface const&);
        };
    }
}
