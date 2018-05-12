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
#include "electroslag/graphics/buffer_opengl.hpp"
#include "electroslag/graphics/frame_buffer_opengl.hpp"
#include "electroslag/graphics/primitive_stream_opengl.hpp"
#include "electroslag/graphics/shader_program_opengl.hpp"
#include "electroslag/graphics/texture_opengl.hpp"
#include "electroslag/graphics/sync_opengl.hpp"

namespace electroslag {
    namespace graphics {
        graphics_interface* get_graphics()
        {
            // TODO: Eventually this code should select an API at build/run time.
            return (get_systems()->get_graphics_opengl());
        }

        graphics_opengl::graphics_opengl()
            : m_mutex(ELECTROSLAG_STRING_AND_HASH("m:graphics_opengl"))
            , m_initialized(false)
        {}

        void graphics_opengl::initialize(
            graphics_initialize_params const* params
            )
        {
            ELECTROSLAG_LOG_MESSAGE("graphics_opengl::initialize");

            if (!params) {
                throw parameter_failure("params");
            }

            // Only allow a single thread to initialize the library in case more than one tries
            // at once.
            threading::lock_guard graphics_lock(&m_mutex);

            if (m_initialized) {
                return;
            }

            m_render_policy.initialize();

            initialize_render_thread(params);
            initialize_sync_thread();

            m_initialized = true;
        }

        void graphics_opengl::shutdown()
        {
            ELECTROSLAG_LOG_MESSAGE("graphics_opengl::shutdown");

            threading::lock_guard graphics_lock(&m_mutex);
            if (m_initialized) {

                m_render_policy.destroy_graphics_objects();

                shutdown_sync_thread();
                shutdown_render_thread();

                m_render_policy.shutdown();

                m_initialized = false;
            }
        }

        bool graphics_opengl::is_initialized() const
        {
            threading::lock_guard graphics_lock(&m_mutex);
            return (m_initialized);
        }

        command_queue_interface::ref graphics_opengl::create_command_queue(
            unsigned long long name_hash
            )
        {
            command_queue::ref new_command_queue(command_queue::create(name_hash));
            m_render_policy.insert_command_queue(new_command_queue);
            return (new_command_queue);
        }

        command_queue_interface::ref graphics_opengl::create_command_queue(
            std::string const& name
            )
        {
            command_queue::ref new_command_queue(command_queue::create(name));
            m_render_policy.insert_command_queue(new_command_queue);
            return (new_command_queue);
        }

        command_queue_interface::ref graphics_opengl::create_command_queue(
            std::string const& name,
            unsigned long long name_hash
            )
        {
            command_queue::ref new_command_queue(command_queue::create(name, name_hash));
            m_render_policy.insert_command_queue(new_command_queue);
            return (new_command_queue);
        }

        command_queue_interface::ref graphics_opengl::create_command_queue(
            std::string const& name,
            command_queue_interface::ref const& insert_after
            )
        {
            command_queue::ref new_command_queue(command_queue::create(name));
            m_render_policy.insert_command_queue(new_command_queue, insert_after);
            return (new_command_queue);
        }

        command_queue_interface::ref graphics_opengl::create_command_queue(
            std::string const& name,
            unsigned long long name_hash,
            command_queue_interface::ref const& insert_after
            )
        {
            command_queue::ref new_command_queue(command_queue::create(name, name_hash));
            m_render_policy.insert_command_queue(new_command_queue, insert_after);
            return (new_command_queue);
        }

        buffer_interface::ref graphics_opengl::create_buffer(
            buffer_descriptor::ref const& buffer_desc
            )
        {
            return (buffer_opengl::create(buffer_desc).cast<buffer_interface>());
        }

        buffer_interface::ref graphics_opengl::create_finished_buffer(
            buffer_descriptor::ref const& buffer_desc
            )
        {
            return (buffer_opengl::create_finished(buffer_desc).cast<buffer_interface>());
        }

        frame_buffer_interface::ref graphics_opengl::create_frame_buffer(
            frame_buffer_attribs const* attribs,
            int width,
            int height
            )
        {
            return (frame_buffer_opengl::create(attribs, width, height).cast<frame_buffer_interface>());
        }

        frame_buffer_interface::ref graphics_opengl::create_finished_frame_buffer(
            frame_buffer_attribs const* attribs,
            int width,
            int height
            )
        {
            return (frame_buffer_opengl::create_finished(attribs, width, height).cast<frame_buffer_interface>());
        }

        primitive_stream_interface::ref graphics_opengl::create_primitive_stream(
            primitive_stream_descriptor::ref const& prim_stream_desc
            )
        {
            return (primitive_stream_opengl::create(prim_stream_desc).cast<primitive_stream_interface>());
        }

        primitive_stream_interface::ref graphics_opengl::create_finished_primitive_stream(
            primitive_stream_descriptor::ref const& prim_stream_desc
        )
        {
            return (primitive_stream_opengl::create_finished(prim_stream_desc).cast<primitive_stream_interface>());
        }

        shader_program_interface::ref graphics_opengl::create_shader_program(
            shader_program_descriptor::ref const& shader_desc
            )
        {
            return (shader_program_opengl::create(shader_desc, shader_field_map::ref::null_ref).cast<shader_program_interface>());
        }

        shader_program_interface::ref graphics_opengl::create_finished_shader_program(
            shader_program_descriptor::ref const& shader_desc
            )
        {
            return (shader_program_opengl::create_finished(shader_desc, shader_field_map::ref::null_ref).cast<shader_program_interface>());
        }

        shader_program_interface::ref graphics_opengl::create_shader_program(
            shader_program_descriptor::ref const& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            return (shader_program_opengl::create(shader_desc, vertex_attrib_field_map).cast<shader_program_interface>());
        }

        shader_program_interface::ref graphics_opengl::create_finished_shader_program(
            shader_program_descriptor::ref const& shader_desc,
            shader_field_map::ref const& vertex_attrib_field_map
            )
        {
            return (shader_program_opengl::create_finished(shader_desc, vertex_attrib_field_map).cast<shader_program_interface>());
        }

        texture_interface::ref graphics_opengl::create_texture(texture_descriptor::ref& texture_desc)
        {
            return (texture_opengl::create(texture_desc).cast<texture_interface>());
        }

        texture_interface::ref graphics_opengl::create_finished_texture(texture_descriptor::ref& texture_desc)
        {
            return (texture_opengl::create_finished(texture_desc).cast<texture_interface>());
        }

        sync_interface::ref graphics_opengl::create_sync()
        {
            return (sync_opengl::create().cast<sync_interface>());
        }

        void graphics_opengl::flush_commands()
        {
            m_render_thread.check_not();

            // First, need to sync up with the render thread to ensure the last batch of commands is finished.
            m_render_thread.wait_for_ready_to_swap();

            // At this point all rendering threads should be stopped; so we can move work over to the render thread.
            m_render_policy.swap();

            // Start the render thread on the new commands we just swapped in.
            m_render_thread.signal_work();
        }

        void graphics_opengl::finish_commands()
        {
            flush_commands();

            // The extra wait ensures the commands swapped in by the flush are finished.
            m_render_thread.wait_for_ready_to_swap();
        }

        void graphics_opengl::initialize_render_thread(graphics_initialize_params const* params)
        {
            ELECTROSLAG_CHECK(!m_render_thread.is_started());
            m_render_thread.spawn(params);
            m_render_thread.wait_for_ready();
        }

        void graphics_opengl::shutdown_render_thread()
        {
            // Process the last batch of commands and destroy the command
            // processing thread.
            if (m_render_thread.is_started()) {
                m_render_thread.signal_exit();
                m_render_thread.wait_for_exit();
                m_render_thread.join();
            }
        }

        void graphics_opengl::initialize_sync_thread()
        {
            ELECTROSLAG_CHECK(!m_sync_thread.is_started());
            m_sync_thread.spawn();
            m_sync_thread.wait_for_ready();
        }

        void graphics_opengl::shutdown_sync_thread()
        {
            // Process the last batch of commands and destroy the command
            // processing thread.
            if (m_sync_thread.is_started()) {
                m_sync_thread.signal_exit();
                m_sync_thread.wait_for_exit();
                m_sync_thread.join();
            }
        }
    }
}
