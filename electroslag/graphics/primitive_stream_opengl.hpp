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
#include "electroslag/graphics/primitive_stream_interface.hpp"
#include "electroslag/graphics/buffer_opengl.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class primitive_stream_opengl : public primitive_stream_interface {
        public:
            typedef reference<primitive_stream_opengl> ref;

            static ref create(
                primitive_stream_descriptor::ref const& prim_stream_desc
                )
            {
                ref new_primitive_stream(new primitive_stream_opengl());
                new_primitive_stream->schedule_async_create(prim_stream_desc);
                return (new_primitive_stream);
            }

            static ref create_finished(
                primitive_stream_descriptor::ref const& prim_stream_desc
            )
            {
                ref new_primitive_stream(new primitive_stream_opengl());
                new_primitive_stream->schedule_async_create_finished(prim_stream_desc);
                return (new_primitive_stream);
            }

            // Implement primitive_stream_interface
            virtual bool is_finished() const
            {
                return (m_is_finished.load(std::memory_order_acquire));
            }

            // Called by context_opengl
            int get_primitive_count() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_prim_count);
            }

            primitive_type get_primitive_type() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_prim_type);
            }

            int get_sizeof_index() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_sizeof_index);
            }

            void bind() const;

        private:
            class create_command : public command {
            public:
                create_command(
                    ref const& prim_stream,
                    primitive_stream_descriptor::ref const& prim_stream_desc
                    )
                    : m_prim_stream(prim_stream)
                    , m_prim_stream_desc(prim_stream_desc)
                {}

                create_command(
                    ref const& prim_stream,
                    primitive_stream_descriptor::ref const& prim_stream_desc,
                    sync_interface::ref const& finish_sync
                    )
                    : m_prim_stream(prim_stream)
                    , m_prim_stream_desc(prim_stream_desc)
                    , m_finish_sync(finish_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_prim_stream;
                primitive_stream_descriptor::ref m_prim_stream_desc;
                sync_interface::ref m_finish_sync;

                // Disallowed operations:
                create_command();
                explicit create_command(create_command const&);
                create_command& operator =(create_command const&);
            };

            class destroy_command : public command {
            public:
                destroy_command(opengl_object_id vertex_array_id)
                    : m_vertex_array_id(vertex_array_id)
                {}

                virtual void execute(context_interface* context);

            private:
                opengl_object_id m_vertex_array_id;

                // Disallowed operations:
                destroy_command();
                explicit destroy_command(destroy_command const&);
                destroy_command& operator =(destroy_command const&);
            };

            typedef std::vector<buffer_opengl::ref> vbo_vector;

            primitive_stream_opengl()
                : m_is_finished(false)
                , m_prim_count(0)
                , m_prim_type(primitive_type_unknown)
                , m_sizeof_index(0)
                , m_vertex_array_id(0)
            {}
            virtual ~primitive_stream_opengl();

            void schedule_async_create(primitive_stream_descriptor::ref const& prim_stream_desc);
            void schedule_async_create_finished(primitive_stream_descriptor::ref const& prim_stream_desc);

            void opengl_create(primitive_stream_descriptor::ref const& prim_stream_desc);

            void opengl_create_vbo(
                primitive_stream_descriptor::ref const& prim_stream_desc,
                primitive_stream_descriptor::const_attribute_iterator i
                );

            static void opengl_destroy(opengl_object_id vertex_array_id);

            std::atomic<bool> m_is_finished;

            vbo_vector m_vbo_vector;
            buffer_opengl::ref m_ibo;

            // Copied from the descriptor on construction
            int m_prim_count;
            primitive_type m_prim_type;
            int m_sizeof_index;

            // Accessed by the graphics command execution thread only
            opengl_object_id m_vertex_array_id;

            // Disallowed operations:
            explicit primitive_stream_opengl(primitive_stream_opengl const&);
            primitive_stream_opengl& operator =(primitive_stream_opengl const&);
        };
    }
}
