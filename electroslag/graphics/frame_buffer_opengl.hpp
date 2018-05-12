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
#include "electroslag/graphics/frame_buffer_interface.hpp"
#include "electroslag/graphics/sync_interface.hpp"

namespace electroslag {
    namespace graphics {
        class frame_buffer_opengl : public frame_buffer_interface {
        public:
            typedef reference<frame_buffer_opengl> ref;

            static ref create(
                frame_buffer_attribs const* attribs,
                int width,
                int height
                )
            {
                ref new_frame_buffer(new frame_buffer_opengl(frame_buffer_type_off_screen, attribs, width, height));
                new_frame_buffer->schedule_async_create();
                return (new_frame_buffer);
            }

            static ref create_finished(
                frame_buffer_attribs const* attribs,
                int width,
                int height
            )
            {
                ref new_frame_buffer(new frame_buffer_opengl(frame_buffer_type_off_screen, attribs, width, height));
                new_frame_buffer->schedule_async_create_finished();
                return (new_frame_buffer);
            }

            // Implement frame_buffer_interface
            virtual bool is_finished() const
            {
                return (m_is_finished.load(std::memory_order_acquire));
            }

            virtual frame_buffer_type get_type() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_type);
            }

            virtual frame_buffer_attribs const* get_attribs() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (&m_attribs);
            }

            virtual int get_width() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_width);
            }

            virtual int get_height() const
            {
                ELECTROSLAG_CHECK(is_finished());
                return (m_height);
            }

            void bind() const;

        private:
            static ref create_display_fbo(frame_buffer_attribs const* attribs)
            {
                ref new_frame_buffer(new frame_buffer_opengl(frame_buffer_type_display, attribs));
                new_frame_buffer->m_is_finished.store(true, std::memory_order_release);
                return (new_frame_buffer);
            }

            class create_command : public command {
            public:
                explicit create_command(ref const& fb)
                    : m_frame_buffer(fb)
                {}

                create_command(
                    ref const& fb,
                    sync_interface::ref const& finish_sync
                    )
                    : m_frame_buffer(fb)
                    , m_finish_sync(finish_sync)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_frame_buffer;
                sync_interface::ref m_finish_sync;

                // Disallowed operations:
                create_command();
                explicit create_command(create_command const&);
                create_command& operator =(create_command const&);
            };

            class destroy_command : public command {
            public:
                destroy_command(
                    opengl_object_id frame_buffer_id,
                    opengl_object_id depth_texture_id,
                    opengl_object_id color_texture_id
                    )
                    : m_frame_buffer_id(frame_buffer_id)
                    , m_depth_texture_id(depth_texture_id)
                    , m_color_texture_id(color_texture_id)
                {}

                virtual void execute(context_interface* context);

            private:
                opengl_object_id m_frame_buffer_id;
                opengl_object_id m_depth_texture_id;
                opengl_object_id m_color_texture_id;

                // Disallowed operations:
                destroy_command();
                explicit destroy_command(destroy_command const&);
                destroy_command& operator =(destroy_command const&);
            };

            class resize_command : public command {
            public:
                resize_command(
                    ref const& fb,
                    ui::window_dimensions const* new_dimensions
                    )
                    : m_frame_buffer(fb)
                    , m_dimensions(*new_dimensions)
                {}

                virtual void execute(context_interface* context);

            private:
                ref m_frame_buffer;
                ui::window_dimensions m_dimensions;

                // Disallowed operations:
                resize_command();
                explicit resize_command(resize_command const&);
                resize_command& operator =(resize_command const&);
            };

            frame_buffer_opengl(
                frame_buffer_type type,
                frame_buffer_attribs const* attribs,
                int width = 0,
                int height = 0
                );
            virtual ~frame_buffer_opengl();

            void schedule_async_create();
            void schedule_async_create_finished();

            void on_window_resize(ui::window_dimensions const* dimensions);

            std::atomic<bool> m_is_finished;

            // Generic members
            frame_buffer_type m_type;
            frame_buffer_attribs m_attribs;
            int m_width;
            int m_height;

            ui::window_interface::size_changed_delegate* m_size_change_delegate;

            // OpenGL is accessed by the graphics command execution thread only
            void opengl_create();

            static void opengl_destroy(
                opengl_object_id frame_buffer_id,
                opengl_object_id depth_texture_id,
                opengl_object_id color_texture_id
                );

            void opengl_set_viewport() const;

            opengl_object_id m_frame_buffer_id;
            opengl_object_id m_depth_texture_id;
            opengl_object_id m_color_texture_id;

            // Disallowed operations:
            frame_buffer_opengl();
            explicit frame_buffer_opengl(frame_buffer_opengl const&);
            frame_buffer_opengl& operator =(frame_buffer_opengl const&);

            // Context needs to create the display fbo.
            friend class context_opengl;
        };
    }
}
