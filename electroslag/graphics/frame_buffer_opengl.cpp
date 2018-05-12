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
#include "electroslag/ui/ui_interface.hpp"
#include "electroslag/graphics/graphics_opengl.hpp"
#include "electroslag/graphics/context_opengl.hpp"
#include "electroslag/graphics/frame_buffer_opengl.hpp"

namespace electroslag {
    namespace graphics {
        frame_buffer_opengl::frame_buffer_opengl(
            frame_buffer_type type,
            frame_buffer_attribs const* attribs,
            int width,
            int height
            )
            : m_is_finished(false)
            , m_type(type)
            , m_attribs(*attribs)
            , m_width(width)
            , m_height(height)
            , m_size_change_delegate(0)
            , m_frame_buffer_id(0)
            , m_depth_texture_id(0)
            , m_color_texture_id(0)
        {
            if (m_attribs.color_format == frame_buffer_color_format_unknown) {
                m_attribs.color_format = frame_buffer_color_format_r8g8b8a8;
            }

            if (m_attribs.depth_stencil_format == frame_buffer_depth_stencil_format_unknown) {
                m_attribs.depth_stencil_format = frame_buffer_depth_stencil_format_d24s8;
            }

            if (m_attribs.msaa == frame_buffer_msaa_unknown) {
                m_attribs.msaa = frame_buffer_msaa_none;
            }

            switch (m_type) {
            case frame_buffer_type_display: {
                // The display frame buffer needs to match the window on-screen; so track it's changes.
                ui::window_interface* window = ui::get_ui()->get_window();

                m_size_change_delegate = ui::window_interface::size_changed_delegate::create_from_method<frame_buffer_opengl, &frame_buffer_opengl::on_window_resize>(this);
                window->size_changed.bind(m_size_change_delegate, event_bind_mode_reference_listener);

                ui::window_dimensions const* dimensions = window->get_dimensions();

                m_width = dimensions->width;
                m_height = dimensions->height;
                break;
            }

            case frame_buffer_type_off_screen: {
                if (!m_width || !m_height) {
                    throw parameter_failure("invalid off-screen frame buffer size");
                }

                // TODO: support MSAA off-screen frame buffers
                if (m_attribs.msaa != frame_buffer_msaa_none) {
                    throw parameter_failure("msaa not supported for off-screen frame buffers");
                }
                break;
            }

            default:
                throw parameter_failure("frame buffer type is invalid");
            }
        }

        void frame_buffer_opengl::schedule_async_create()
        {
            ELECTROSLAG_CHECK(m_type != frame_buffer_type_display);
            graphics_interface* g = get_graphics();

            if (g->get_render_thread()->is_running()) {
                opengl_create();
            }
            else {
                g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                    ref(this)
                    );
            }
        }

        void frame_buffer_opengl::schedule_async_create_finished()
        {
            ELECTROSLAG_CHECK(m_type != frame_buffer_type_display);
            graphics_interface* g = get_graphics();
            ELECTROSLAG_CHECK(!g->get_render_thread()->is_running());

            sync_interface::ref finish_sync = g->create_sync();

            g->get_render_policy()->get_system_command_queue()->enqueue_command<create_command>(
                ref(this),
                finish_sync
                );

            finish_sync->wait();
        }

        frame_buffer_opengl::~frame_buffer_opengl()
        {
            if (m_size_change_delegate) {
                ui::window_interface* window = ui::get_ui()->get_window();
                window->size_changed.unbind(m_size_change_delegate);
                delete m_size_change_delegate;
                m_size_change_delegate = 0;
            }

            if (m_is_finished.load(std::memory_order_acquire)) {
                m_is_finished.store(false, std::memory_order_release);

                graphics_interface* g = get_graphics();
                if (g->get_render_thread()->is_running()) {
                    opengl_destroy(
                        m_frame_buffer_id,
                        m_depth_texture_id,
                        m_color_texture_id
                        );
                }
                else {
                    // This command can run completely asynchronously to "this" objects destruction.
                    g->get_render_policy()->get_system_command_queue()->enqueue_command<destroy_command>(
                        m_frame_buffer_id,
                        m_depth_texture_id,
                        m_color_texture_id
                        );
                }

                m_frame_buffer_id = 0;
                m_depth_texture_id = 0;
                m_color_texture_id = 0;
            }
            else {
                ELECTROSLAG_LOG_WARN("frame_buffer_opengl destructor before finished");
            }
        }

        void frame_buffer_opengl::create_command::execute(
            context_interface* context
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_frame_buffer->opengl_create();

            if (m_finish_sync.is_valid()) {
                context->set_sync_point(m_finish_sync);
            }
        }

        void frame_buffer_opengl::destroy_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            frame_buffer_opengl::opengl_destroy(
                m_frame_buffer_id,
                m_depth_texture_id,
                m_color_texture_id
                );
        }

        void frame_buffer_opengl::resize_command::execute(
#if !defined(ELECTROSLAG_BUILD_SHIP)
            context_interface* context
#else
            context_interface*
#endif
            )
        {
            ELECTROSLAG_CHECK(context);
            get_graphics()->get_render_thread()->check();

            m_frame_buffer->m_width = m_dimensions.width;
            m_frame_buffer->m_height = m_dimensions.height;
            m_frame_buffer->opengl_set_viewport();
        }

        void frame_buffer_opengl::on_window_resize(
            ui::window_dimensions const* dimensions
            )
        {
            ELECTROSLAG_CHECK(m_type == frame_buffer_type_display);
            get_graphics()->get_render_policy()->get_system_command_queue()->enqueue_command<resize_command>(
                ref(this),
                dimensions
                );
        }

        void frame_buffer_opengl::opengl_create()
        {
            ELECTROSLAG_CHECK(!is_finished());
            ELECTROSLAG_CHECK(m_type != frame_buffer_type_display);

            opengl_object_id frame_buffer_id = 0;
            gl::GenFramebuffers(1, &frame_buffer_id);
            context_opengl::check_opengl_error();

            ELECTROSLAG_CHECK(!m_frame_buffer_id);
            m_frame_buffer_id = frame_buffer_id;

            gl::BindFramebuffer(gl::FRAMEBUFFER, frame_buffer_id);
            context_opengl::check_opengl_error();

            // Create a depth/stencil texture, if needed.
            GLenum depth_stencil_format = 0;
            GLenum depth_stencil_attachment = gl::DEPTH_ATTACHMENT;
            switch (m_attribs.depth_stencil_format) {
            case frame_buffer_depth_stencil_format_none:
                m_depth_texture_id = 0;
                break;

            case frame_buffer_depth_stencil_format_d16:
                depth_stencil_format = gl::DEPTH_COMPONENT16;
                break;

            case frame_buffer_depth_stencil_format_d24:
                depth_stencil_format = gl::DEPTH_COMPONENT24;
                break;

            case frame_buffer_depth_stencil_format_d32:
                if (!m_attribs.depth_stencil_shadow_hint) {
                    depth_stencil_format = gl::DEPTH_COMPONENT32;
                }
                else {
                    depth_stencil_format = gl::DEPTH_COMPONENT32F;
                }
                break;

            case frame_buffer_depth_stencil_format_d24s8:
                if (!m_attribs.depth_stencil_shadow_hint) {
                    depth_stencil_format = gl::DEPTH24_STENCIL8;
                    depth_stencil_attachment = gl::DEPTH_STENCIL_ATTACHMENT;
                }
                else {
                    // Lie a little; the depth component will be 32bpp, not 24.
                    depth_stencil_format = gl::DEPTH32F_STENCIL8;
                    depth_stencil_attachment = gl::DEPTH_STENCIL_ATTACHMENT;
                }
                break;

            default:
                throw parameter_failure("off-screen frame buffer depth format is invalid");
            }

            if (depth_stencil_format) {
                opengl_object_id depth_texture_id = 0;

                gl::GenTextures(1, &depth_texture_id);
                context_opengl::check_opengl_error();

                ELECTROSLAG_CHECK(!m_depth_texture_id);
                m_depth_texture_id = depth_texture_id;

                gl::BindTexture(gl::TEXTURE_2D, depth_texture_id);
                context_opengl::check_opengl_error();

                gl::TexStorage2D(
                    gl::TEXTURE_2D,
                    1,
                    depth_stencil_format,
                    m_width,
                    m_height
                    );
                context_opengl::check_opengl_error();

                gl::FramebufferTexture2D(gl::FRAMEBUFFER, depth_stencil_attachment, gl::TEXTURE_2D, depth_texture_id, 0);
                context_opengl::check_opengl_error();
            }

            // Create the color texture, if needed.
            GLenum color_format = 0;
            GLenum color_attachment = gl::COLOR_ATTACHMENT0;
            switch (m_attribs.color_format) {
            case frame_buffer_color_format_none:
                m_color_texture_id = 0;
                break;

            case frame_buffer_color_format_r8g8b8a8:
                color_format = gl::RGBA8;
                break;

            case frame_buffer_color_format_r8g8b8a8_srgb:
                color_format = gl::SRGB8_ALPHA8;
                break;

            default:
                throw parameter_failure("off-screen frame buffer color format is invalid");
            }

            if (color_format) {
                opengl_object_id color_texture_id = 0;
                gl::GenTextures(1, &color_texture_id);
                context_opengl::check_opengl_error();

                ELECTROSLAG_CHECK(!m_color_texture_id);
                m_color_texture_id = color_texture_id;

                gl::BindTexture(gl::TEXTURE_2D, color_texture_id);
                context_opengl::check_opengl_error();

                gl::TexStorage2D(
                    gl::TEXTURE_2D,
                    1,
                    color_format,
                    m_width,
                    m_height
                    );
                context_opengl::check_opengl_error();

                gl::FramebufferTexture2D(gl::FRAMEBUFFER, color_attachment, gl::TEXTURE_2D, color_texture_id, 0);
                context_opengl::check_opengl_error();
            }

            // Extra check to ensure we have a valid frame buffer
            GLenum frame_buffer_status = gl::CheckFramebufferStatus(gl::FRAMEBUFFER);
            if (frame_buffer_status != gl::FRAMEBUFFER_COMPLETE) {
                throw opengl_api_failure("Frame buffer is incomplete", frame_buffer_status);
            }

            m_is_finished.store(true, std::memory_order_release);
        }

        // static
        void frame_buffer_opengl::opengl_destroy(
            opengl_object_id frame_buffer_id,
            opengl_object_id depth_texture_id,
            opengl_object_id color_texture_id
            )
        {
            if (frame_buffer_id) {
                gl::DeleteFramebuffers(1, &frame_buffer_id);
            }
            if (depth_texture_id) {
                gl::DeleteTextures(1, &depth_texture_id);
            }
            if (color_texture_id) {
                gl::DeleteTextures(1, &color_texture_id);
            }
        }

        void frame_buffer_opengl::bind() const
        {
            get_graphics()->get_render_thread()->check();
            ELECTROSLAG_CHECK(is_finished());

            if (m_type == frame_buffer_type_off_screen) {
                ELECTROSLAG_CHECK(m_frame_buffer_id);
                gl::BindFramebuffer(gl::FRAMEBUFFER, m_frame_buffer_id);
            }
            else {
                gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
            }
            context_opengl::check_opengl_error();

            if (m_attribs.color_format == frame_buffer_color_format_r8g8b8a8_srgb) {
                gl::Enable(gl::FRAMEBUFFER_SRGB);
            }
            else {
                gl::Disable(gl::FRAMEBUFFER_SRGB);
            }
            context_opengl::check_opengl_error();

            opengl_set_viewport();
        }

        void frame_buffer_opengl::opengl_set_viewport() const
        {
            gl::Viewport(0, 0, m_width, m_height);
            context_opengl::check_opengl_error();
        }
    }
}
