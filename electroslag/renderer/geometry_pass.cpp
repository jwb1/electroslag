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
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/renderer/geometry_pass.hpp"
#include "electroslag/renderer/renderer.hpp"
#include "electroslag/renderer/camera.hpp"

namespace electroslag {
    namespace renderer {
        geometry_pass::geometry_pass(
            pipeline_type type,
            unsigned long long camera_hash,
            graphics::command_queue_interface::ref const& insert_after,
            char const* queue_name_prefix
            )
            : m_type(type)
            , m_scene_created_delegate(0)
            , m_camera_hash(camera_hash)
            , m_transparents_mutex(ELECTROSLAG_STRING_AND_HASH("m:forward_pass_transparents"))
        {
            // Create all of the necessary command queues.
            graphics::graphics_interface* g = graphics::get_graphics();

            std::string command_queue_name_prefix("cq:");
            command_queue_name_prefix += queue_name_prefix;
            command_queue_name_prefix += ":";

            std::string command_queue_name;

            command_queue_name = command_queue_name_prefix + "setup_queue";
            m_setup_queue = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                insert_after
                );

            command_queue_name = command_queue_name_prefix + "draw_opaques_queue_near";
            m_draw_opaques_queues[opaque_depth_near] = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                m_setup_queue
                );

            command_queue_name = command_queue_name_prefix + "draw_opaques_queue_middle";
            m_draw_opaques_queues[opaque_depth_middle] = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                m_draw_opaques_queues[opaque_depth_near]
                );

            command_queue_name = command_queue_name_prefix + "draw_opaques_queue_far";
            m_draw_opaques_queues[opaque_depth_far] = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                m_draw_opaques_queues[opaque_depth_middle]
                );

            command_queue_name = command_queue_name_prefix + "draw_opaques_queue_billboard";
            m_draw_opaques_queues[opaque_depth_billboard] = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                m_draw_opaques_queues[opaque_depth_far]
                );

            command_queue_name = command_queue_name_prefix + "draw_opaques_queue_skybox";
            m_draw_opaques_queues[opaque_depth_skybox] = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                m_draw_opaques_queues[opaque_depth_billboard]
                );

            command_queue_name = command_queue_name_prefix + "draw_transparents_queue";
            m_draw_transparents_queue = g->create_command_queue(
                command_queue_name.c_str(),
                hash_string_runtime(command_queue_name),
                insert_after
                );

            // Listen for scene creation.
            m_scene_created_delegate = renderer::scene_created_delegate::create_from_method<geometry_pass, &geometry_pass::on_scene_created>(this);
            get_renderer_internal()->scene_created.bind(m_scene_created_delegate, event_bind_mode_reference_listener);
        }

        geometry_pass::~geometry_pass()
        {
            // Destroy listeners.
            if (m_scene_created_delegate) {
                get_renderer_internal()->scene_created.unbind(m_scene_created_delegate);
                delete m_scene_created_delegate;
                m_scene_created_delegate = 0;
            }
        }

        bool geometry_pass::locate_field_source(graphics::shader_field const* /*field*/, void const** /*field_source*/) const
        {
            return (false);
        }

        pass_interface::pass_render_work_item::ref geometry_pass::make_render_work_item(
            frame_details* this_frame_details
            )
        {
            return (threading::get_frame_thread_pool()->enqueue_work_item<geometry_pass_render_work_item>(
                this,
                this_frame_details
                ).cast<pass_render_work_item>());
        }

        void geometry_pass::geometry_pass_render_work_item::execute()
        {
            geometry_pass* this_pass = static_cast<geometry_pass*>(m_this_pass);
            ELECTROSLAG_CHECK(this_pass->m_camera.is_valid());

            // The passes camera needs to setup in a queue before any draws.
            this_pass->m_setup_queue->enqueue_command<camera::setup_camera_command>(
                this_pass->m_camera,
                m_this_frame_details
                );

            // Enqueue the command that will draw the transparent meshes that are cached.
            this_pass->m_draw_transparents_queue->enqueue_command<draw_transparents_command>(
                this_pass,
                m_this_frame_details
                );
        }

        void geometry_pass::on_scene_created(scene::ref const& new_scene)
        {
            camera::ref candidate_camera(new_scene->find_camera(m_camera_hash));

            if (candidate_camera.is_valid()) {
                if (m_camera.is_valid()) {
                    throw load_object_failure("duplicate forward pass cameras");
                }

                m_camera = candidate_camera;
            }
        }

        // TODO: Initial guesses for values; assumes far clipping plane at 1000.0f. Tune.
        // static
        float const geometry_pass::depth_thresholds[opaque_depth_count] = {
            50.0f,   // opaque_depth_near
            250.0f,  // opaque_depth_middle
            600.0f,  // opaque_depth_far
            0.0f,    // opaque_depth_billboard
            0.0f     // opaque_depth_skybox 
        };

        void geometry_pass::render_mesh_in_pass(mesh_interface::ref& mesh, frame_details* this_frame_details)
        {
            float camera_distance = 0.0f;
            if (!m_camera.is_valid() || m_camera->view_frustum_cull(mesh, &camera_distance)) {
                return;
            }

            mesh->compute_local_to_clip(pipeline_type_forward_geometry, m_camera->get_world_to_clip());

            mesh->write_dynamic_ubo(pipeline_type_forward_geometry, this_frame_details);

            if (mesh->is_semi_transparent(pipeline_type_forward_geometry)) {
                threading::lock_guard lock_transparents(&m_transparents_mutex);

                transparent_meshes::iterator r(m_transparents.begin());
                transparent_meshes::iterator prev(m_transparents.before_begin());
                while (r != m_transparents.end()) {
                    if (r->depth > camera_distance) {
                        m_transparents.emplace_after(prev, camera_distance, mesh);
                        break;
                    }
                    prev = r;
                    ++r;
                }
            }
            else {
                opaque_depth q = opaque_depth_unknown;
                if (mesh->is_skybox()) {
                    q = opaque_depth_skybox;
                }
                else {
                    // TODO: Levels of detail
                    if (camera_distance < depth_thresholds[opaque_depth_near]) {
                        q = opaque_depth_near;
                    }
                    else if (camera_distance < depth_thresholds[opaque_depth_middle]) {
                        q = opaque_depth_middle;
                    }
                    else if (camera_distance < depth_thresholds[opaque_depth_far]) {
                        q = opaque_depth_far;
                    }
                    else {
                        q = opaque_depth_billboard;
                    }
                }

                m_draw_opaques_queues[q]->enqueue_command<mesh_interface::mesh_draw_command>(
                    m_type,
                    mesh,
                    this_frame_details
                    );
            }
        }

        void geometry_pass::draw_transparents(graphics::context_interface* context, frame_details* this_frame_details)
        {
            // Iterate the transparent meshes; should be sorted back to front already.
            threading::lock_guard lock_transparents(&m_transparents_mutex);
            transparent_meshes::iterator r(m_transparents.begin());
            while (r != m_transparents.end()) {
                r->mesh->draw(context, m_type, this_frame_details);
                ++r;
            }
        }
    }
}
