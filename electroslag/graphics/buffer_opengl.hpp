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
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/graphics/graphics_types.hpp"
#include "electroslag/graphics/buffer_descriptor.hpp"
#include "electroslag/graphics/buffer_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class buffer_opengl : public buffer_interface {
        public:
            typedef reference<buffer_opengl> ref;

            static ref create(buffer_descriptor::ref const& buffer_desc)
            {
                ref new_buffer(new buffer_opengl());
                new_buffer->schedule_async_create(buffer_desc);
                return (new_buffer);
            }

            static ref create_finished(buffer_descriptor::ref const& buffer_desc)
            {
                ref new_buffer(new buffer_opengl());
                new_buffer->schedule_async_create_finished(buffer_desc);
                return (new_buffer);
            }

            // Implement buffer_interface
            virtual bool is_finished() const
            {
                return (m_is_finished.load(std::memory_order_acquire));
            }

            virtual byte* map(int offset = 0, int bytes = -1);
            virtual void unmap(int offset = 0, int bytes = -1);

            virtual void flush_cpu_writes(int offset = 0, int bytes = -1);
            virtual void flush_gpu_writes(int offset = 0, int bytes = -1);

            // Called by context_opengl
            void bind(GLenum target) const;
            void bind_to_index(GLenum target, int index) const;
            void bind_range_to_index(GLenum target, int index, int start, int end = -1) const;

        private:
            // This class needs to do manipulations on buffers that require them to be
            // bound to a target, but aren't really specific to any target. This
            // target has no special semantics, so we can avoid using targets that actually
            // mean something.
            static GLenum const general_buffer_manipulation_target = gl::COPY_READ_BUFFER;

            class create_command : public command {
            public:
                create_command(
                    ref const& buffer,
                    buffer_descriptor::ref const& buffer_desc
                    )
                    : m_buffer(buffer)
                    , m_buffer_desc(buffer_desc)
                {}

                create_command(
                    ref const& buffer,
                    buffer_descriptor::ref const& buffer_desc,
                    sync_interface::ref const& finish_sync
                    )
                    : m_buffer(buffer)
                    , m_buffer_desc(buffer_desc)
                    , m_finish_sync(finish_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_buffer;
                buffer_descriptor::ref m_buffer_desc;
                sync_interface::ref m_finish_sync;

                // Disallowed operations:
                create_command();
                explicit create_command(create_command const&);
                create_command& operator =(create_command const&);
            };

            class destroy_command : public command {
            public:
                explicit destroy_command(
                    opengl_object_id id
                    )
                    : m_id(id)
                {}

                virtual void execute(context_interface* context);

            private:
                opengl_object_id m_id;

                // Disallowed operations:
                destroy_command();
                explicit destroy_command(destroy_command const&);
                destroy_command& operator =(destroy_command const&);
            };

            class flush_gpu_writes_command : public command {
            public:
                flush_gpu_writes_command(
                    ref const& buffer
                    )
                    : m_buffer(buffer)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_buffer;

                // Disallowed operations:
                flush_gpu_writes_command();
                explicit flush_gpu_writes_command(flush_gpu_writes_command const&);
                flush_gpu_writes_command& operator =(flush_gpu_writes_command const&);
            };

            class flush_cpu_writes_command : public command {
            public:
                flush_cpu_writes_command(
                    ref const& buffer,
                    int offset,
                    int bytes
                    )
                    : m_buffer(buffer)
                    , m_offset(offset)
                    , m_bytes(bytes)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_buffer;
                int m_offset;
                int m_bytes;

                // Disallowed operations:
                flush_cpu_writes_command();
                explicit flush_cpu_writes_command(flush_cpu_writes_command const&);
                flush_cpu_writes_command& operator =(flush_cpu_writes_command const&);
            };

            buffer_opengl()
                : m_is_finished(false)
                , m_mapped_pointer(0)
                , m_size(0)
                , m_id(0)
            {}
            virtual ~buffer_opengl();

            void schedule_async_create(buffer_descriptor::ref const& buffer_desc);
            void schedule_async_create_finished(buffer_descriptor::ref const& buffer_desc);

            // Operations performed on the graphics command execution thread
            void opengl_create(buffer_descriptor::ref const& buffer_desc);
            static void opengl_destroy(opengl_object_id id);

            void opengl_flush_gpu_writes();
            void opengl_flush_cpu_writes(int offset, int bytes);

            std::atomic<bool> m_is_finished;

            // These fields are setup during creation and then not changed again.
            byte* m_mapped_pointer;
            int m_size;
            opengl_object_id m_id;

            struct map_flags {
                map_flags()
                    : flush_gpu_writes_on_map(false)
                    , flush_cpu_writes_on_unmap(false)
                {}

                bool flush_gpu_writes_on_map:1;
                bool flush_cpu_writes_on_unmap:1;
            } m_map_flags;

            // Disallowed operations:
            explicit buffer_opengl(buffer_opengl const&);
            buffer_opengl& operator =(buffer_opengl const&);
        };
    }
}
