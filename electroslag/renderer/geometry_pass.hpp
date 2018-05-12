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
#include "electroslag/graphics/frame_buffer_interface.hpp"
#include "electroslag/renderer/mesh_interface.hpp"
#include "electroslag/renderer/pass_interface.hpp"
#include "electroslag/renderer/camera.hpp"
#include "electroslag/renderer/renderer.hpp"

namespace electroslag {
    namespace renderer {
        class renderer;
        class geometry_pass : public pass_interface {
        public:
            static ref create(
                pipeline_type type,
                unsigned long long camera_hash,
                graphics::command_queue_interface::ref const& insert_after,
                char const* queue_name_prefix
                )
            {
                return (ref(new geometry_pass(type, camera_hash, insert_after, queue_name_prefix)));
            }

            virtual ~geometry_pass();

            // Implement field_source_interface
            virtual bool locate_field_source(graphics::shader_field const* field, void const** field_source) const;

            // Implement pass_interface
            virtual pass_type get_pass_type() const
            {
                return (pass_type_geometry);
            }

            virtual pipeline_type get_pipeline_type() const
            {
                return (m_type);
            }

            virtual pass_render_work_item::ref make_render_work_item(
                frame_details* this_frame_details
                );

            virtual void render_mesh_in_pass(mesh_interface::ref& mesh, frame_details* this_frame_details);

        protected:
            geometry_pass(
                pipeline_type type,
                unsigned long long camera_hash,
                graphics::command_queue_interface::ref const& insert_after,
                char const* queue_name_prefix
                );

        private:
            void on_scene_created(scene::ref const& new_scene);

            // The renderer generates a thread pool command for each pass.
            class geometry_pass_render_work_item
                : public pass_render_work_item {
            public:
                geometry_pass_render_work_item(
                    geometry_pass* this_pass,
                    frame_details* this_frame_details
                    )
                    : pass_render_work_item(this_pass, this_frame_details)
                {}

                virtual void execute();
            };

            // Graphics commands talk to the graphics context.
            class draw_transparents_command : public graphics::command {
            public:
                draw_transparents_command(
                    geometry_pass* this_pass,
                    frame_details* this_frame_details
                    )
                    : m_this_pass(this_pass)
                    , m_this_frame_details(this_frame_details)
                {}

                virtual void execute(graphics::context_interface* context)
                {
                    m_this_pass->draw_transparents(context, m_this_frame_details);
                }

            private:
                geometry_pass* m_this_pass;
                frame_details* m_this_frame_details;
            };

            void draw_transparents(
                graphics::context_interface* context,
                frame_details* this_frame_details
                );

            // Depth sorting ranges
            enum opaque_depth {
                opaque_depth_unknown = -1,
                opaque_depth_near = 0,
                opaque_depth_middle = 1,
                opaque_depth_far = 2,
                opaque_depth_billboard = 3,
                opaque_depth_skybox = 4,

                opaque_depth_count // Ensure this is the last enum entry
            };

            static float const depth_thresholds[opaque_depth_count];

            pipeline_type m_type;

            graphics::command_queue_interface::ref m_setup_queue;
            graphics::command_queue_interface::ref m_draw_opaques_queues[opaque_depth_count];
            graphics::command_queue_interface::ref m_draw_transparents_queue;

            renderer::scene_created_delegate* m_scene_created_delegate;

            unsigned long long m_camera_hash;
            camera::ref m_camera;

            typedef std::vector<graphics::frame_buffer_interface::ref> frame_buffer_vector;
            frame_buffer_vector m_input_buffers;
            frame_buffer_vector m_output_buffers;

            threading::mutex m_transparents_mutex;
            struct depth_sorted_mesh {
                depth_sorted_mesh(
                    float new_depth,
                    mesh_interface::ref& new_mesh
                    )
                    : depth(new_depth)
                    , mesh(new_mesh)
                {}

                float depth;
                mesh_interface::ref mesh;
            };
            // Because there is a ref in the depth_sorted_mesh, if this list was
            // a vector, the insert in the middle would be potentially very expensive.
            typedef std::forward_list<depth_sorted_mesh> transparent_meshes;
            transparent_meshes m_transparents;

            // Disallowed operations:
            geometry_pass();
            explicit geometry_pass(geometry_pass const&);
            geometry_pass& operator =(geometry_pass const&);
        };
    }
}
