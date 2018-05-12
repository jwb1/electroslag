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
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/graphics/context_opengl.hpp"
#include "electroslag/graphics/buffer_opengl.hpp"

namespace electroslag {
    namespace graphics {
        void buffer_opengl::schedule_async_create(buffer_descriptor::ref const& buffer_desc)
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_create(buffer_desc);
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                    ref(this),
                    buffer_desc
                    );
            }
        }

        void buffer_opengl::schedule_async_create_finished(buffer_descriptor::ref const& buffer_desc)
        {
            graphics_interface* g = get_graphics();
            ELECTROSLAG_CHECK(!g->get_render_thread()->is_running());

            sync_interface::ref finish_sync = g->create_sync();

            g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                ref(this),
                buffer_desc,
                finish_sync
                );

            finish_sync->wait();
        }

        buffer_opengl::~buffer_opengl()
        {
            if (m_is_finished.load(std::memory_order_acquire)) {
                m_is_finished.store(false, std::memory_order_release);

                graphics_interface* g = get_graphics();

                if (g->get_render_thread()->is_running()) {
                    opengl_destroy(m_id);
                }
                else {
                    // This command can run completely asynchronously to "this" objects destruction.
                    g->get_render_policy()->get_system_command_queue()->enqueue_command<destroy_command>(
                        m_id
                        );
                }

                m_mapped_pointer = 0;
                m_size = 0;
                m_id = 0;
            }
            else {
                ELECTROSLAG_LOG_WARN("buffer_opengl destructor before finished");
            }
        }

        // static
        void buffer_opengl::opengl_destroy(
            opengl_object_id id
            )
        {
            if (id) {
                gl::BindBuffer(general_buffer_manipulation_target, id);
                gl::UnmapBuffer(general_buffer_manipulation_target);

                gl::DeleteBuffers(1, &id);
            }
        }

        void buffer_opengl::opengl_create(buffer_descriptor::ref const& buffer_desc)
        {
            ELECTROSLAG_CHECK(!is_finished());
            buffer_memory_map memory_map = buffer_desc->get_buffer_memory_map();
            buffer_memory_caching memory_caching = buffer_desc->get_buffer_memory_caching();

            GLbitfield flags;
            GLbitfield map_flags;
            switch (memory_map) {
            case buffer_memory_map_static:
                flags = 0;
                map_flags = 0;
                break;

            case buffer_memory_map_read:
                flags = gl::MAP_READ_BIT | gl::MAP_PERSISTENT_BIT;
                map_flags = flags;
                break;

            case buffer_memory_map_write:
                flags = gl::MAP_WRITE_BIT | gl::MAP_PERSISTENT_BIT;
                map_flags = flags | gl::MAP_INVALIDATE_BUFFER_BIT;
                break;

            case buffer_memory_map_read_write:
                flags = gl::MAP_READ_BIT | gl::MAP_WRITE_BIT | gl::MAP_PERSISTENT_BIT;
                map_flags = flags;
                break;

            default:
                throw parameter_failure("buffer mode is invalid");
            }
            // NVidia recommends against gl::MAP_UNSYNCHRONIZED_BIT. Maybe we don't care?
            // Maybe we need/want it on other GPUs.

            if (memory_map != buffer_memory_map_static) {
                if (memory_caching == buffer_memory_caching_coherent) {
                    flags |= gl::MAP_COHERENT_BIT;
                    map_flags |= gl::MAP_COHERENT_BIT;
                }
                else if (memory_caching == buffer_memory_caching_noncoherent) {
                    map_flags |= gl::MAP_FLUSH_EXPLICIT_BIT;

                    // Remember where we need flushing one way or the other.
                    if (flags & gl::MAP_READ_BIT) {
                        m_map_flags.flush_gpu_writes_on_map = true;
                    }
                    if (flags & gl::MAP_WRITE_BIT) {
                        m_map_flags.flush_cpu_writes_on_unmap = true;
                    }
                }
                else {
                    throw parameter_failure("buffer memory caching type is invalid");
                }
            }
            else {
                if (memory_caching != buffer_memory_caching_static) {
                    throw parameter_failure("buffer memory caching type is invalid");
                }
            }

            opengl_object_id buffer_id = 0;
            gl::GenBuffers(1, &buffer_id);
            context_opengl::check_opengl_error();

            ELECTROSLAG_CHECK(!m_id);
            m_id = buffer_id;

            gl::BindBuffer(general_buffer_manipulation_target, buffer_id);
            context_opengl::check_opengl_error();

            if (buffer_desc->has_initialized_data()) {
                ELECTROSLAG_CHECK(!buffer_desc->has_uninitialized_data());

                referenced_buffer_interface::accessor accessor(buffer_desc->get_initialized_data());
                m_size = accessor.get_sizeof();

                gl::BufferStorage(general_buffer_manipulation_target, accessor.get_sizeof(), accessor.get_pointer(), flags);
                context_opengl::check_opengl_error();
            }
            else {
                ELECTROSLAG_CHECK(buffer_desc->has_uninitialized_data());

                m_size = buffer_desc->get_uninitialized_data_size();

                gl::BufferStorage(general_buffer_manipulation_target, m_size, 0, flags);
                context_opengl::check_opengl_error();
            }

            if (memory_map != buffer_memory_map_static) {
                m_mapped_pointer = static_cast<byte*>(gl::MapBufferRange(
                    general_buffer_manipulation_target,
                    0,
                    m_size,
                    map_flags
                    ));
                context_opengl::check_opengl_error();
                ELECTROSLAG_CHECK(m_mapped_pointer);
            }
            else {
                m_mapped_pointer = 0;
            }

#if !defined(ELECTROSLAG_BUILD_SHIP)
            if (buffer_desc->has_name_string()) {
                std::string buffer_name(buffer_desc->get_name());
                gl::ObjectLabel(
                    gl::BUFFER,
                    buffer_id,
                    static_cast<GLsizei>(buffer_name.length()),
                    buffer_name.c_str()
                    );
                context_opengl::check_opengl_error();
            }
#endif

            m_is_finished.store(true, std::memory_order_release);
        }

        void buffer_opengl::opengl_flush_gpu_writes()
        {
            ELECTROSLAG_CHECK(is_finished());

            gl::MemoryBarrier(gl::CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            context_opengl::check_opengl_error();

            gl::Finish();
            context_opengl::check_opengl_error();
        }

        void buffer_opengl::opengl_flush_cpu_writes(int offset, int bytes)
        {
            ELECTROSLAG_CHECK(is_finished());

            gl::BindBuffer(general_buffer_manipulation_target, m_id);
            context_opengl::check_opengl_error();

            gl::FlushMappedBufferRange(general_buffer_manipulation_target, offset, bytes);
            context_opengl::check_opengl_error();
        }

        void buffer_opengl::create_command::execute(
            context_interface* context
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_buffer->opengl_create(m_buffer_desc);

            if (m_finish_sync.is_valid()) {
                context->set_sync_point(m_finish_sync);
            }
        }

        void buffer_opengl::destroy_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            buffer_opengl::opengl_destroy(m_id);
        }

        void buffer_opengl::flush_gpu_writes_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_buffer->opengl_flush_gpu_writes();
        }

        void buffer_opengl::flush_cpu_writes_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_buffer->opengl_flush_cpu_writes(m_offset, m_bytes);
        }

        byte* buffer_opengl::map(int offset, int bytes)
        {
            ELECTROSLAG_CHECK(is_finished());
            ELECTROSLAG_CHECK(m_mapped_pointer && m_size > 0);
            ELECTROSLAG_CHECK(offset >= 0 && offset < m_size);
            if (bytes <= 0) {
                bytes = m_size - offset;
            }
            ELECTROSLAG_CHECK(offset + bytes <= m_size);

            if (m_map_flags.flush_gpu_writes_on_map) {
                flush_gpu_writes(offset, bytes);
            }

            return (m_mapped_pointer + offset);
        }

        void buffer_opengl::unmap(int offset, int bytes)
        {
            ELECTROSLAG_CHECK(is_finished());
            if (m_map_flags.flush_cpu_writes_on_unmap) {
                flush_cpu_writes(offset, bytes);
            }
        }

        void buffer_opengl::flush_cpu_writes(int offset, int bytes)
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_flush_cpu_writes(offset, bytes);
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<flush_cpu_writes_command>(
                    this,
                    offset,
                    bytes
                    );
            }
        }

        void buffer_opengl::flush_gpu_writes(int, int)
        {
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_flush_gpu_writes();
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<flush_gpu_writes_command>(this);
            }
        }

        void buffer_opengl::bind(GLenum target) const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            gl::BindBuffer(target, m_id);
            context_opengl::check_opengl_error();
        }

        void buffer_opengl::bind_to_index(GLenum target, int index) const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            gl::BindBufferBase(target, index, m_id);
            context_opengl::check_opengl_error();
        }

        void buffer_opengl::bind_range_to_index(GLenum target, int index, int start, int end) const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            ELECTROSLAG_CHECK(m_id);
            ELECTROSLAG_CHECK(start >= 0);
            // The start value must be a multiple of context_opengl::get_min_ubo_offset_alignment()

            int range_size = 0;
            if (end > start) {
                range_size = end - start;
            }
            else {
                range_size = m_size - start;
            }

            gl::BindBufferRange(target, index, m_id, start, range_size);
            context_opengl::check_opengl_error();
        }
    }
}
