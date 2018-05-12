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
#include "electroslag/systems.hpp"
#include "electroslag/graphics/context_interface.hpp"
#include "electroslag/animation/property_manager.hpp"
#include "electroslag/renderer/geometry_pass.hpp"

namespace electroslag {
    namespace renderer {
        renderer* get_renderer_internal()
        {
            return (get_systems()->get_renderer());
        }

        renderer_interface* get_renderer()
        {
            return (get_renderer_internal());
        }

        renderer::renderer()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:renderer"))
            , m_destroyed_delegate(0)
            , m_size_changed_delegate(0)
            , m_paused_changed_delegate(0)
            , m_frame_delegate(0)
            , m_geometry_pass_count(0)
            , m_current_frame_index(0)
            , m_initialized(false)
        {}

        bool renderer::locate_field_source(graphics::shader_field const* /*field*/, void const** /*field_source*/) const
        {
            return (false);
        }

        void renderer::initialize()
        {
            threading::lock_guard renderer_lock(&m_mutex);

            if (m_initialized) {
                return;
            }

            graphics::graphics_interface* g = graphics::get_graphics();

            // Create common command queues.
            m_pre_frame_queue = g->create_command_queue(ELECTROSLAG_STRING_AND_HASH("cq:pre_frame"));
            m_post_frame_queue = g->create_command_queue(ELECTROSLAG_STRING_AND_HASH("cq:post_frame"));

            // Set up member objects.
            m_ubo_manager.initialize();
            m_primitive_stream_manager.initialize();
            m_texture_manager.initialize();
            m_pipeline_manager.initialize();
            m_shader_program_manager.initialize();

            // Setup per-frame data
            for (int i = 0; i < per_frame_count; ++i) {
                m_per_frame_details[i].reset();
                m_per_frame_details[i].sync = graphics::get_graphics()->create_sync();
            }

            // Forward rendering currently only involves a single pass.
            m_passes.emplace_back(geometry_pass::create(
                pipeline_type_forward_geometry,
                hash_string("forward_camera"),
                m_pre_frame_queue,
                "forward_geometry"
                ));
            m_geometry_pass_count = 1;

            // Hook up windows events.
            ui::window_interface* window = ui::get_ui()->get_window();

            m_destroyed_delegate = ui::window_interface::destroyed_delegate::create_from_method<renderer, &renderer::on_window_destroyed>(this);
            window->destroyed.bind(m_destroyed_delegate, event_bind_mode_reference_listener);

            m_size_changed_delegate = ui::window_interface::size_changed_delegate::create_from_method<renderer, &renderer::on_window_size_changed>(this);
            window->size_changed.bind(m_size_changed_delegate, event_bind_mode_reference_listener);

            m_paused_changed_delegate = ui::window_interface::paused_changed_delegate::create_from_method<renderer, &renderer::on_window_paused_changed>(this);
            window->paused_changed.bind(m_paused_changed_delegate, event_bind_mode_reference_listener);

            m_frame_delegate = ui::window_interface::frame_delegate::create_from_method<renderer, &renderer::on_window_frame>(this);
            window->frame.bind(m_frame_delegate, event_bind_mode_reference_listener);

            m_initialized = true;
        }

        void renderer::shutdown()
        {
            on_window_destroyed();
        }

        bool renderer::is_initialized() const
        {
            threading::lock_guard renderer_lock(&m_mutex);
            return (m_initialized);
        }

        scene::ref renderer::create_instances(instance_descriptor::ref const& desc)
        {
            threading::lock_guard renderer_lock(&m_mutex);
            check_initialized();

            // Later versions of C++ allow emplace_back a return value.
            scene::ref new_scene(*m_scenes.emplace(m_scenes.end(), scene::create(desc)));
            scene_created.signal(new_scene);
            return (new_scene);
        }

        void renderer::on_window_destroyed()
        {
            threading::lock_guard renderer_lock(&m_mutex);

            if (m_initialized) {
                destroyed.signal();

                ui::window_interface* window = ui::get_ui()->get_window();
                graphics::graphics_interface* g = graphics::get_graphics();

                // Destroy listeners.
                if (m_destroyed_delegate) {
                    window->destroyed.unbind(m_destroyed_delegate);
                    delete m_destroyed_delegate;
                    m_destroyed_delegate = 0;
                }

                if (m_size_changed_delegate) {
                    window->size_changed.unbind(m_size_changed_delegate);
                    delete m_size_changed_delegate;
                    m_size_changed_delegate = 0;
                }

                if (m_paused_changed_delegate) {
                    window->paused_changed.unbind(m_paused_changed_delegate);
                    delete m_paused_changed_delegate;
                    m_paused_changed_delegate = 0;
                }

                if (m_frame_delegate) {
                    window->frame.unbind(m_frame_delegate);
                    delete m_frame_delegate;
                    m_frame_delegate = 0;
                }

                // Release all scene and pass references.
                m_passes.clear();
                m_scenes.clear();

                // We can't be using any UBO memory any more.
                m_pipeline_manager.shutdown();
                m_ubo_manager.shutdown();
                m_texture_manager.shutdown();
                m_primitive_stream_manager.shutdown();
                m_shader_program_manager.shutdown();

                // Shut down graphics.
                g->finish_commands();
                g->shutdown();

                // Now we can dump command queue references.
                m_pre_frame_queue.reset();
                m_post_frame_queue.reset();

                m_initialized = false;
            }
        }

        void renderer::on_window_size_changed(ui::window_dimensions const* /*dimensions*/)
        {

        }

        void renderer::on_window_paused_changed(bool /*paused*/)
        {

        }

        void renderer::on_window_frame(int millisec_elapsed)
        {
            threading::lock_guard renderer_lock(&m_mutex);
            check_initialized();

            // Short circuit for nothing to do.
            if (m_scenes.size() == 0) {
                return;
            }

            // Rotate through the per-frame data structures.
            m_current_frame_index++;
            if (m_current_frame_index == per_frame_count) {
                m_current_frame_index = 0;
            }
            frame_details* this_frame_details = &m_per_frame_details[m_current_frame_index];
            this_frame_details->reset();
            this_frame_details->millisec_elapsed = millisec_elapsed;
            this_frame_details->r = this;

            // Ensure the per-frame data we intend to use is no longer in use by the GPU.
            if (this_frame_details->sync->is_set()) {
                this_frame_details->sync->wait(&renderer_lock);
            }
            this_frame_details->sync->clear();

            // Tick any pipelines still working on initialization.
            m_pipeline_manager.prepare_pipelines_for_frame(this_frame_details);

            // Wait for UBO space to be available. (Likely the fence is already passed.)
            m_ubo_manager.prepare_dynamic_ubo_for_frame(this_frame_details);

            // Apply all pending controller modifications.
            animation::get_property_manager_internal()->apply_controller_changes();

            // Generate a thread pool work item to update and transform all scenes.
            scene_vector::iterator s(m_scenes.begin());
            while (s != m_scenes.end()) {
                (*s)->make_transform_work_item(this_frame_details);
                ++s;
            }

            // Generate a thread pool work item to generate graphics calls for each pass and each scene.
            pass_vector::iterator p(m_passes.begin());
            while (p != m_passes.end()) {
                (*p)->make_render_work_item(this_frame_details);
                ++p;
            }

            s = m_scenes.begin();
            while (s != m_scenes.end()) {
                (*s)->make_render_work_item(this_frame_details);
                ++s;
            }
        }

        void renderer::finish_rendering_mesh(frame_details* this_frame_details)
        {
            int completed_meshes = this_frame_details->completed_meshes.fetch_add(1);
            if (completed_meshes + 1 == this_frame_details->total_meshes) {
                // Spawn a last command that will execute at the end of the frame.
                this_frame_details->r->m_post_frame_queue->enqueue_command<finish_drawing_command>(this_frame_details);

                // Send all graphics commands generated by the frame handlers to the render thread.
                graphics::get_graphics()->flush_commands();
            }
        }

        void renderer::finish_drawing_command::execute(graphics::context_interface* context)
        {
            // The execution of all drawing commands by the graphics API calls should be done.

            // The UBO allocator uses a fence to track when the UBO space is ready to re-use, set it.
            context->set_sync_point(m_this_frame_details->sync);

            // Swap all drawing on screen!
            context->swap();
        }
    }
}
