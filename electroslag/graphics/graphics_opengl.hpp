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
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/sync_thread_opengl.hpp"

namespace electroslag {
    namespace graphics {
        class graphics_opengl : public graphics_interface {
        public:
            graphics_opengl();

            // Implement graphics_interface
            virtual void initialize(graphics_initialize_params const* params);
            virtual void shutdown();

            virtual bool is_initialized() const;

            virtual command_queue_interface::ref create_command_queue(
                unsigned long long name_hash
                );

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name
                );

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                unsigned long long name_hash
                );

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                command_queue_interface::ref const& insert_after
                );

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                unsigned long long name_hash,
                command_queue_interface::ref const& insert_after
                );

            virtual buffer_interface::ref create_buffer(
                buffer_descriptor::ref const& buffer_desc
                );

            virtual buffer_interface::ref create_finished_buffer(
                buffer_descriptor::ref const& buffer_desc
                );

            virtual frame_buffer_interface::ref create_frame_buffer(
                frame_buffer_attribs const* attribs,
                int width = 0,
                int height = 0
                );

            virtual frame_buffer_interface::ref create_finished_frame_buffer(
                frame_buffer_attribs const* attribs,
                int width = 0,
                int height = 0
                );

            virtual primitive_stream_interface::ref create_primitive_stream(
                primitive_stream_descriptor::ref const& prim_stream_desc
                );

            virtual primitive_stream_interface::ref create_finished_primitive_stream(
                primitive_stream_descriptor::ref const& prim_stream_desc
                );

            virtual shader_program_interface::ref create_shader_program(
                shader_program_descriptor::ref const& shader_desc
                );

            virtual shader_program_interface::ref create_finished_shader_program(
                shader_program_descriptor::ref const& shader_desc
                );

            virtual shader_program_interface::ref create_shader_program(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );

            virtual shader_program_interface::ref create_finished_shader_program(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );

            virtual texture_interface::ref create_texture(
                texture_descriptor::ref& texture_desc
                );

            virtual texture_interface::ref create_finished_texture(
                texture_descriptor::ref& texture_desc
                );

            virtual sync_interface::ref create_sync();

            virtual void flush_commands();
            virtual void finish_commands();

            virtual render_policy* get_render_policy()
            {
                return (&m_render_policy);
            }

            virtual render_thread* get_render_thread()
            {
                return (&m_render_thread);
            }

            virtual bool has_context_capability()
            {
                return (m_render_thread.get_context_capability() != 0);
            }

            virtual context_capability_interface* get_context_capability()
            {
                return (m_render_thread.get_context_capability());
            }

            virtual bool has_graphics_debugger()
            {
                graphics_debugger_interface* gd = m_render_thread.get_graphics_debugger();
                if (gd) {
                    return (gd->is_graphics_debugger_attached());
                }
                else {
                    return (false);
                }
            }

            virtual graphics_debugger_interface* get_graphics_debugger()
            {
                return (m_render_thread.get_graphics_debugger());
            }

            virtual void finish_setting_sync(sync_interface::ref& s)
            {
                m_sync_thread.wait_for_sync(s);
            }

#if defined(_WIN32)
            // This is signaled by context_opengl when it's now time for sub-contexts
            // to create their own GL context. It has to be here so it is constructed
            // early enough for the sub-context constructors to use it.
            mutable sub_context_opengl::hglrc_created_event hglrc_created;
#endif
        private:
            mutable threading::mutex m_mutex;

            render_policy m_render_policy;

            // The commands stored in the render policy are processed via the render
            // thread; the only thread allowed to call the OS graphics itself.
            void initialize_render_thread(graphics_initialize_params const* params);
            void shutdown_render_thread();
            render_thread m_render_thread;

            // The sync thread coordinates signaling sync_interface objects as needed.
            void initialize_sync_thread();
            void shutdown_sync_thread();
            sync_thread_opengl m_sync_thread;

            bool m_initialized;

            // Disallowed operations:
            explicit graphics_opengl(graphics_opengl const&);
            graphics_opengl& operator =(graphics_opengl const&);
        };
    }
}
