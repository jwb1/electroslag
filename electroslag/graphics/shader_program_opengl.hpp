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
#include "electroslag/graphics/primitive_stream_descriptor.hpp"
#include "electroslag/graphics/shader_program_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class shader_program_opengl : public shader_program_interface  {
        public:
            typedef reference<shader_program_opengl> ref;

            static ref create(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                )
            {
                ref new_shader(new shader_program_opengl());
                new_shader->schedule_async_create(shader_desc, vertex_attrib_field_map);
                return (new_shader);
            }

            static ref create_finished(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                )
            {
                ref new_shader(new shader_program_opengl());
                new_shader->schedule_async_create_finished(shader_desc, vertex_attrib_field_map);
                return (new_shader);
            }

            // Implement shader_program_interface
            virtual bool is_finished() const
            {
                return (m_is_finished.load(std::memory_order_acquire));
            }

            virtual shader_program_descriptor::ref const& get_descriptor() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_descriptor);
            }

            // Called by context_opengl
            void bind() const;

        private:
            class create_command : public command {
            public:
                create_command(
                    ref const& shader,
                    shader_program_descriptor::ref const& shader_desc,
                    shader_field_map::ref const& vertex_attrib_field_map
                    )
                    : m_shader(shader)
                    , m_shader_desc(shader_desc)
                    , m_vertex_attrib_field_map(vertex_attrib_field_map)
                {}

                create_command(
                    ref const& shader,
                    shader_program_descriptor::ref const& shader_desc,
                    shader_field_map::ref const& vertex_attrib_field_map,
                    sync_interface::ref const& finish_sync
                    )
                    : m_shader(shader)
                    , m_shader_desc(shader_desc)
                    , m_vertex_attrib_field_map(vertex_attrib_field_map)
                    , m_finish_sync(finish_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_shader;
                shader_program_descriptor::ref m_shader_desc;
                shader_field_map::ref m_vertex_attrib_field_map;
                sync_interface::ref m_finish_sync;

                // Disallowed operations:
                create_command();
                explicit create_command(create_command const&);
                create_command& operator =(create_command const&);
            };

            class destroy_command : public command {
            public:
                destroy_command(
                    opengl_object_id vertex_part,
                    opengl_object_id tessellation_control_part,
                    opengl_object_id tessellation_evaluation_part,
                    opengl_object_id geometry_part,
                    opengl_object_id fragment_part,
                    opengl_object_id compute_part,
                    opengl_object_id program
                    )
                    : m_vertex_part(vertex_part)
                    , m_tessellation_control_part(tessellation_control_part)
                    , m_tessellation_evaluation_part(tessellation_evaluation_part)
                    , m_geometry_part(geometry_part)
                    , m_fragment_part(fragment_part)
                    , m_compute_part(compute_part)
                    , m_program(program)
                {}

                virtual void execute(context_interface* context);

            private:
                opengl_object_id m_vertex_part;
                opengl_object_id m_tessellation_control_part;
                opengl_object_id m_tessellation_evaluation_part;
                opengl_object_id m_geometry_part;
                opengl_object_id m_fragment_part;
                opengl_object_id m_compute_part;
                opengl_object_id m_program;

                // Disallowed operations:
                destroy_command();
                explicit destroy_command(destroy_command const&);
                destroy_command& operator =(destroy_command const&);
            };

            shader_program_opengl()
                : m_is_finished(false)
                , m_vertex_part(0)
                , m_tessellation_control_part(0)
                , m_tessellation_evaluation_part(0)
                , m_geometry_part(0)
                , m_fragment_part(0)
                , m_compute_part(0)
                , m_program(0)
            {}
            virtual ~shader_program_opengl();

            void schedule_async_create(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );
            void schedule_async_create_finished(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );

            // Operations performed on the graphics command execution thread
            void opengl_create(
                shader_program_descriptor::ref const& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );

            void opengl_compile_shader_program(shader_program_descriptor::ref& shader_desc);

            static void opengl_compile_shader_stage(
                shader_stage_descriptor::ref& stage_descriptor,
                opengl_object_id* output_part,
                std::string const& shader_defines
                );

            void opengl_set_attrib_metadata(
                shader_program_descriptor::ref& shader_desc,
                shader_field_map::ref const& vertex_attrib_field_map
                );

            void opengl_link_shader_program();

            void opengl_gather_ubo_metadata(
                shader_program_descriptor::ref& shader_desc
                );
            void opengl_gather_stage_ubo_metadata(
                shader_stage_descriptor::ref& stage_descriptor
                );

            void opengl_finish_create(shader_program_descriptor::ref& shader_desc);

            static void opengl_destroy(
                opengl_object_id vertex_part,
                opengl_object_id tessellation_control_part,
                opengl_object_id tessellation_evaluation_part,
                opengl_object_id geometry_part,
                opengl_object_id fragment_part,
                opengl_object_id compute_part,
                opengl_object_id program
                );

#if !defined(ELECTROSLAG_BUILD_SHIP)
            static void opengl_interface_query(opengl_object_id program);
#endif

            std::atomic<bool> m_is_finished;
            shader_program_descriptor::ref m_descriptor;

            // Accessed by the graphics command execution thread only
            opengl_object_id m_vertex_part;
            opengl_object_id m_tessellation_control_part;
            opengl_object_id m_tessellation_evaluation_part;
            opengl_object_id m_geometry_part;
            opengl_object_id m_fragment_part;
            opengl_object_id m_compute_part;
            opengl_object_id m_program;

            // Disallowed operations:
            explicit shader_program_opengl(shader_program_opengl const&);
            shader_program_opengl& operator =(shader_program_opengl const&);
        };
    }
}
