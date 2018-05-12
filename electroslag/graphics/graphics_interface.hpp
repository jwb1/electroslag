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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/shader_field_map.hpp"
#include "electroslag/graphics/command_queue_interface.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/graphics/buffer_interface.hpp"
#include "electroslag/graphics/frame_buffer_interface.hpp"
#include "electroslag/graphics/shader_program_descriptor.hpp"
#include "electroslag/graphics/shader_program_interface.hpp"
#include "electroslag/graphics/primitive_stream_descriptor.hpp"
#include "electroslag/graphics/primitive_stream_interface.hpp"
#include "electroslag/graphics/texture_descriptor.hpp"
#include "electroslag/graphics/texture_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"
#include "electroslag/graphics/render_policy.hpp"
#include "electroslag/graphics/render_thread.hpp"
#include "electroslag/graphics/graphics_debugger_interface.hpp"
#include "electroslag/graphics/context_capability_interface.hpp"

namespace electroslag {
    namespace graphics {
        class graphics_interface {
        public:
            virtual void initialize(graphics_initialize_params const* params) = 0;
            virtual void shutdown() = 0;

            virtual bool is_initialized() const = 0;

            void check_not_initialized() const
            {
                ELECTROSLAG_CHECK(!is_initialized());
            }

            void check_initialized() const
            {
                ELECTROSLAG_CHECK(is_initialized());
            }

            virtual command_queue_interface::ref create_command_queue(
                unsigned long long name_hash
                ) = 0;

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name
                ) = 0;

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                unsigned long long name_hash
                ) = 0;

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                command_queue_interface::ref const& insert_after
                ) = 0;

            virtual command_queue_interface::ref create_command_queue(
                std::string const& name,
                unsigned long long name_hash,
                command_queue_interface::ref const& insert_after
                ) = 0;

            virtual buffer_interface::ref create_buffer(
                buffer_descriptor::ref const& buffer_desc
                ) = 0;

            virtual buffer_interface::ref create_finished_buffer(
                buffer_descriptor::ref const& buffer_desc
                ) = 0;

            virtual frame_buffer_interface::ref create_frame_buffer(
                frame_buffer_attribs const* attribs,
                int width = 0,
                int height = 0
                ) = 0;

            virtual frame_buffer_interface::ref create_finished_frame_buffer(
                frame_buffer_attribs const* attribs,
                int width = 0,
                int height = 0
                ) = 0;

            virtual primitive_stream_interface::ref create_primitive_stream(
                primitive_stream_descriptor::ref const& prim_stream_desc
                ) = 0;

            virtual primitive_stream_interface::ref create_finished_primitive_stream(
                primitive_stream_descriptor::ref const& prim_stream_desc
                ) = 0;

            virtual shader_program_interface::ref create_shader_program(
                shader_program_descriptor::ref const& shader_desc
                ) = 0;

            virtual shader_program_interface::ref create_finished_shader_program(
                shader_program_descriptor::ref const& shader_desc
                ) = 0;

            virtual shader_program_interface::ref create_shader_program(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                ) = 0;

            virtual shader_program_interface::ref create_finished_shader_program(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                ) = 0;

            virtual texture_interface::ref create_texture(
                texture_descriptor::ref& texture_desc
                ) = 0;

            virtual texture_interface::ref create_finished_texture(
                texture_descriptor::ref& texture_desc
                ) = 0;

            virtual sync_interface::ref create_sync() = 0;

            virtual void flush_commands() = 0;
            virtual void finish_commands() = 0;

            virtual render_policy* get_render_policy() = 0;
            virtual render_thread* get_render_thread() = 0;

            virtual bool has_context_capability() = 0;
            virtual context_capability_interface* get_context_capability() = 0;

            virtual bool has_graphics_debugger() = 0;
            virtual graphics_debugger_interface* get_graphics_debugger() = 0;

            virtual void finish_setting_sync(sync_interface::ref& s) = 0;
        };

        graphics_interface* get_graphics();
    }
}
