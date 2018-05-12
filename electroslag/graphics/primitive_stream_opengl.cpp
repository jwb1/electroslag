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
#include "electroslag/graphics/primitive_stream_opengl.hpp"
#include "electroslag/graphics/graphics_interface.hpp"
#include "electroslag/graphics/context_opengl.hpp"
#include "electroslag/graphics/buffer_opengl.hpp"
#include "electroslag/serialize/database.hpp"

namespace electroslag {
    namespace graphics {
        void primitive_stream_opengl::schedule_async_create(
            primitive_stream_descriptor::ref const& prim_stream_desc
            )
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_create(prim_stream_desc);
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                    ref(this),
                    prim_stream_desc
                    );
            }
        }

        void primitive_stream_opengl::schedule_async_create_finished(
            primitive_stream_descriptor::ref const& prim_stream_desc
            )
        {
            graphics_interface* g = get_graphics();
            ELECTROSLAG_CHECK(!g->get_render_thread()->is_running());

            sync_interface::ref finish_sync = g->create_sync();

            g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                ref(this),
                prim_stream_desc,
                finish_sync
                );

            finish_sync->wait();
        }

        primitive_stream_opengl::~primitive_stream_opengl()
        {
            if (m_is_finished.load(std::memory_order_acquire)) {
                m_is_finished.store(false, std::memory_order_release);

                m_ibo.reset();

                vbo_vector::iterator i(m_vbo_vector.begin());
                while (i != m_vbo_vector.end()) {
                    i->reset();
                    ++i;
                }
                m_vbo_vector.clear();

                if (m_vertex_array_id) {
                    graphics_interface* g = get_graphics();

                    if (g->get_render_thread()->is_running()) {
                        opengl_destroy(m_vertex_array_id);
                    }
                    else {
                        // This command can run completely asynchronously to "this" objects destruction.
                        g->get_render_policy()->get_system_command_queue()->enqueue_command<destroy_command>(
                            m_vertex_array_id
                            );
                    }

                    m_vertex_array_id = 0;
                }
            }
            else {
                ELECTROSLAG_LOG_WARN("primitive_stream_opengl destructor before finished");
            }
        }

        // static
        void primitive_stream_opengl::opengl_destroy(
            opengl_object_id vertex_array_id
            )
        {
            if (vertex_array_id) {
                gl::DeleteVertexArrays(1, &vertex_array_id);
            }
        }

        void primitive_stream_opengl::opengl_create(
            primitive_stream_descriptor::ref const& prim_stream_desc
            )
        {
            ELECTROSLAG_CHECK(!is_finished());
            m_prim_count = prim_stream_desc->get_prim_count();
            m_prim_type = prim_stream_desc->get_prim_type();
            m_sizeof_index = prim_stream_desc->get_sizeof_index();

            m_ibo = buffer_opengl::create(prim_stream_desc->get_index_buffer());

            opengl_object_id vertex_array_id = 0;
            gl::GenVertexArrays(1, &vertex_array_id);
            context_opengl::check_opengl_error();

            ELECTROSLAG_CHECK(!m_vertex_array_id);
            m_vertex_array_id = vertex_array_id;

            gl::BindVertexArray(vertex_array_id);
            context_opengl::check_opengl_error();

            m_vbo_vector.resize(prim_stream_desc->get_attribute_count());
            primitive_stream_descriptor::const_attribute_iterator i(prim_stream_desc->begin_attributes());
            while (i != prim_stream_desc->end_attributes()) {
                opengl_create_vbo(prim_stream_desc, i);

                vertex_attribute const* vert_attrib = (*i);
                shader_field const* vert_field = vert_attrib->get_field();

                int index = vert_field->get_index();
                field_type type = vert_field->get_field_type();

                intptr_t attrib_offset = vert_field->get_offset();
                gl::VertexAttribPointer(
                    index,
                    field_type_util::get_order(type),
                    field_type_util::get_opengl_type(type),
                    gl::FALSE_,
                    vert_attrib->get_stride(),
                    reinterpret_cast<void const*>(attrib_offset)
                    );
                context_opengl::check_opengl_error();

                gl::EnableVertexAttribArray(index);
                context_opengl::check_opengl_error();

                ++i;
            }

            m_ibo.cast<buffer_opengl>()->bind(gl::ELEMENT_ARRAY_BUFFER);

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (prim_stream_desc->has_name_string()) {
                std::string prim_stream_name(prim_stream_desc->get_name());
                gl::ObjectLabel(
                    gl::VERTEX_ARRAY,
                    m_vertex_array_id,
                    static_cast<GLsizei>(prim_stream_name.length()),
                    prim_stream_name.c_str()
                    );
                context_opengl::check_opengl_error();
            }
#endif

            m_is_finished.store(true, std::memory_order_release);
        }

        void primitive_stream_opengl::opengl_create_vbo(
            primitive_stream_descriptor::ref const& prim_stream_desc,
            primitive_stream_descriptor::const_attribute_iterator i
            )
        {
            // A buffer might be referred to by many attributes, but should only be created once.
            int dupe_index = -1;
            primitive_stream_descriptor::const_attribute_iterator j(prim_stream_desc->begin_attributes());
            while (j != i) {
                if ((*j)->get_buffer()->get_hash() == (*i)->get_buffer()->get_hash()) {
                    dupe_index = static_cast<int>(j - prim_stream_desc->begin_attributes());
                    break;
                }
                else {
                    ++j;
                }
            }

            int current_index = static_cast<int>(i - prim_stream_desc->begin_attributes());
            if (dupe_index != -1) {
                m_vbo_vector[current_index] = m_vbo_vector[dupe_index];
            }
            else {
                m_vbo_vector[current_index] = buffer_opengl::create((*i)->get_buffer());
            }
            m_vbo_vector[current_index].cast<buffer_opengl>()->bind(gl::ARRAY_BUFFER);
        }

        void primitive_stream_opengl::create_command::execute(
            context_interface* context
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_prim_stream->opengl_create(m_prim_stream_desc);

            if (m_finish_sync.is_valid()) {
                context->set_sync_point(m_finish_sync);
            }
        }

        void primitive_stream_opengl::destroy_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            primitive_stream_opengl::opengl_destroy(m_vertex_array_id);
        }

        void primitive_stream_opengl::bind() const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            ELECTROSLAG_CHECK(m_vertex_array_id);
            gl::BindVertexArray(m_vertex_array_id);
            context_opengl::check_opengl_error();
        }
    }
}
